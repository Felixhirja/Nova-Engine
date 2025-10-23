#include "SVGSurfaceLoader.h"

#ifdef USE_SDL
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {

constexpr float kPi = 3.14159265358979323846f;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

struct StyleProperties {
    bool fillNone = false;
    std::optional<Color> fill;
    std::optional<float> fillOpacity;
    std::optional<float> opacity;

    void Apply(const StyleProperties& other) {
        if (other.fillNone) {
            fillNone = true;
            fill.reset();
        }
        if (other.fill.has_value()) {
            fill = other.fill;
            fillNone = false;
        }
        if (other.fillOpacity.has_value()) {
            fillOpacity = other.fillOpacity;
        }
        if (other.opacity.has_value()) {
            opacity = other.opacity;
        }
    }
};

struct Shape {
    std::vector<std::vector<Vec2>> subpaths;
    Color color;
};

std::string Trim(const std::string& str) {
    size_t begin = 0;
    while (begin < str.size() && std::isspace(static_cast<unsigned char>(str[begin]))) {
        ++begin;
    }
    size_t end = str.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(str[end - 1]))) {
        --end;
    }
    return str.substr(begin, end - begin);
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

bool ParseFloat(const std::string& token, float& out) {
    if (token.empty()) return false;
    char* end = nullptr;
    out = std::strtof(token.c_str(), &end);
    return end != token.c_str();
}

std::optional<float> ParseLength(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::string trimmed = Trim(token);
    float value = 0.0f;
    if (!ParseFloat(trimmed, value)) return std::nullopt;
    return value;
}

bool StartsWith(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

bool ParseColorString(const std::string& value, Color& outColor) {
    std::string lower = ToLower(Trim(value));
    if (lower == "none") {
        return false;
    }
    if (StartsWith(lower, "rgba(")) {
        std::string inner = lower.substr(5, lower.size() - 6);
        std::vector<float> comps;
        std::stringstream ss(inner);
        std::string item;
        while (std::getline(ss, item, ',')) {
            float v = 0.0f;
            if (ParseFloat(Trim(item), v)) {
                comps.push_back(v);
            }
        }
        if (comps.size() == 4) {
            outColor.r = comps[0] / 255.0f;
            outColor.g = comps[1] / 255.0f;
            outColor.b = comps[2] / 255.0f;
            outColor.a = comps[3];
            outColor.a = std::clamp(outColor.a, 0.0f, 1.0f);
            outColor.r = std::clamp(outColor.r, 0.0f, 1.0f);
            outColor.g = std::clamp(outColor.g, 0.0f, 1.0f);
            outColor.b = std::clamp(outColor.b, 0.0f, 1.0f);
            return true;
        }
    } else if (StartsWith(lower, "rgb(")) {
        std::string inner = lower.substr(4, lower.size() - 5);
        std::vector<float> comps;
        std::stringstream ss(inner);
        std::string item;
        while (std::getline(ss, item, ',')) {
            float v = 0.0f;
            if (ParseFloat(Trim(item), v)) {
                comps.push_back(v);
            }
        }
        if (comps.size() == 3) {
            outColor.r = comps[0] / 255.0f;
            outColor.g = comps[1] / 255.0f;
            outColor.b = comps[2] / 255.0f;
            outColor.a = 1.0f;
            outColor.r = std::clamp(outColor.r, 0.0f, 1.0f);
            outColor.g = std::clamp(outColor.g, 0.0f, 1.0f);
            outColor.b = std::clamp(outColor.b, 0.0f, 1.0f);
            return true;
        }
    } else if (StartsWith(lower, "#")) {
        std::string hex = lower.substr(1);
        auto hexToFloat = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
            return 0;
        };
        if (hex.size() == 3) {
            int r = hexToFloat(hex[0]);
            int g = hexToFloat(hex[1]);
            int b = hexToFloat(hex[2]);
            outColor.r = (r * 17) / 255.0f;
            outColor.g = (g * 17) / 255.0f;
            outColor.b = (b * 17) / 255.0f;
            outColor.a = 1.0f;
            return true;
        } else if (hex.size() == 6 || hex.size() == 8) {
            auto parseByte = [&](size_t idx) {
                return (hexToFloat(hex[idx]) << 4) | hexToFloat(hex[idx + 1]);
            };
            int r = parseByte(0);
            int g = parseByte(2);
            int b = parseByte(4);
            outColor.r = r / 255.0f;
            outColor.g = g / 255.0f;
            outColor.b = b / 255.0f;
            if (hex.size() == 8) {
                int a = parseByte(6);
                outColor.a = a / 255.0f;
            } else {
                outColor.a = 1.0f;
            }
            return true;
        }
    } else {
        static const std::unordered_map<std::string, Color> kNamedColors = {
            {"black", {0.0f, 0.0f, 0.0f, 1.0f}},
            {"white", {1.0f, 1.0f, 1.0f, 1.0f}},
            {"red", {1.0f, 0.0f, 0.0f, 1.0f}},
            {"green", {0.0f, 1.0f, 0.0f, 1.0f}},
            {"blue", {0.0f, 0.0f, 1.0f, 1.0f}},
            {"yellow", {1.0f, 1.0f, 0.0f, 1.0f}},
            {"cyan", {0.0f, 1.0f, 1.0f, 1.0f}},
            {"magenta", {1.0f, 0.0f, 1.0f, 1.0f}},
            {"gray", {0.5f, 0.5f, 0.5f, 1.0f}},
        };
        auto it = kNamedColors.find(lower);
        if (it != kNamedColors.end()) {
            outColor = it->second;
            return true;
        }
    }
    return false;
}

StyleProperties ParseStyleDeclarations(const std::string& text) {
    StyleProperties props;
    std::stringstream ss(text);
    std::string decl;
    while (std::getline(ss, decl, ';')) {
        auto colon = decl.find(':');
        if (colon == std::string::npos) continue;
        std::string name = ToLower(Trim(decl.substr(0, colon)));
        std::string value = Trim(decl.substr(colon + 1));
        if (name == "fill") {
            Color c;
            if (ParseColorString(value, c)) {
                props.fill = c;
                props.fillNone = false;
            } else {
                props.fillNone = true;
                props.fill.reset();
            }
        } else if (name == "fill-opacity") {
            float v = 0.0f;
            if (ParseFloat(value, v)) {
                props.fillOpacity = std::clamp(v, 0.0f, 1.0f);
            }
        } else if (name == "opacity") {
            float v = 0.0f;
            if (ParseFloat(value, v)) {
                props.opacity = std::clamp(v, 0.0f, 1.0f);
            }
        }
    }
    return props;
}

std::unordered_map<std::string, std::string> ParseAttributes(const std::string& tag) {
    std::unordered_map<std::string, std::string> attributes;
    size_t pos = 0;
    while (pos < tag.size() && !std::isspace(static_cast<unsigned char>(tag[pos]))) {
        ++pos;
    }
    while (pos < tag.size()) {
        while (pos < tag.size() && std::isspace(static_cast<unsigned char>(tag[pos]))) ++pos;
        size_t startName = pos;
        while (pos < tag.size() && tag[pos] != '=' && !std::isspace(static_cast<unsigned char>(tag[pos]))) ++pos;
        if (pos >= tag.size()) break;
        std::string name = tag.substr(startName, pos - startName);
        while (pos < tag.size() && tag[pos] != '=') ++pos;
        if (pos >= tag.size()) break;
        ++pos; // skip '='
        while (pos < tag.size() && std::isspace(static_cast<unsigned char>(tag[pos]))) ++pos;
        if (pos >= tag.size()) break;
        char quote = tag[pos];
        if (quote != '"' && quote != 0x27) {
            // Bare value
            size_t startValue = pos;
            while (pos < tag.size() && !std::isspace(static_cast<unsigned char>(tag[pos]))) ++pos;
            attributes[ToLower(name)] = tag.substr(startValue, pos - startValue);
        } else {
            ++pos;
            size_t startValue = pos;
            while (pos < tag.size() && tag[pos] != quote) ++pos;
            if (pos >= tag.size()) break;
            attributes[ToLower(name)] = tag.substr(startValue, pos - startValue);
            ++pos; // skip closing quote
        }
    }
    return attributes;
}

std::vector<Vec2> BuildRectangle(float x, float y, float w, float h) {
    std::vector<Vec2> pts = {
        {x, y},
        {x + w, y},
        {x + w, y + h},
        {x, y + h},
    };
    return pts;
}

std::vector<Vec2> BuildEllipse(float cx, float cy, float rx, float ry, int segments = 48) {
    std::vector<Vec2> pts;
    if (rx <= 0.0f || ry <= 0.0f) return pts;
    pts.reserve(segments);
    for (int i = 0; i < segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float angle = t * 2.0f * kPi;
        pts.push_back({cx + std::cos(angle) * rx, cy + std::sin(angle) * ry});
    }
    return pts;
}

std::vector<Vec2> ParsePointList(const std::string& text) {
    std::vector<Vec2> pts;
    const char* ptr = text.c_str();
    while (*ptr) {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) ++ptr;
        if (!*ptr) break;
        char* end = nullptr;
        float x = std::strtof(ptr, &end);
        if (end == ptr) break;
        ptr = end;
        while (*ptr && (*ptr == ',' || std::isspace(static_cast<unsigned char>(*ptr)))) ++ptr;
        if (!*ptr) break;
        float y = std::strtof(ptr, &end);
        if (end == ptr) break;
        ptr = end;
        pts.push_back({x, y});
    }
    return pts;
}

void AddCubic(std::vector<Vec2>& path, const Vec2& p0, const Vec2& p1, const Vec2& p2, const Vec2& p3, int segments = 16) {
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float it = 1.0f - t;
        float x = it * it * it * p0.x + 3.0f * it * it * t * p1.x + 3.0f * it * t * t * p2.x + t * t * t * p3.x;
        float y = it * it * it * p0.y + 3.0f * it * it * t * p1.y + 3.0f * it * t * t * p2.y + t * t * t * p3.y;
        path.push_back({x, y});
    }
}

void AddQuadratic(std::vector<Vec2>& path, const Vec2& p0, const Vec2& p1, const Vec2& p2, int segments = 12) {
    for (int i = 1; i <= segments; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(segments);
        float it = 1.0f - t;
        float x = it * it * p0.x + 2.0f * it * t * p1.x + t * t * p2.x;
        float y = it * it * p0.y + 2.0f * it * t * p1.y + t * t * p2.y;
        path.push_back({x, y});
    }
}

struct PathParseResult {
    std::vector<std::vector<Vec2>> subpaths;
};

PathParseResult ParsePath(const std::string& data) {
    PathParseResult result;
    const char* ptr = data.c_str();
    Vec2 current{0.0f, 0.0f};
    Vec2 start{0.0f, 0.0f};
    Vec2 prevControlC{0.0f, 0.0f};
    Vec2 prevControlQ{0.0f, 0.0f};
    bool hasPrevControlC = false;
    bool hasPrevControlQ = false;
    char command = 0;
    std::vector<Vec2>* activePath = nullptr;

    auto readFloat = [&](float& value) -> bool {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) ++ptr;
        if (!*ptr) return false;
        char* end = nullptr;
        value = std::strtof(ptr, &end);
        if (end == ptr) return false;
        ptr = end;
        return true;
    };

    auto ensureActivePath = [&]() {
        if (!activePath) {
            result.subpaths.emplace_back();
            activePath = &result.subpaths.back();
            activePath->push_back(current);
        }
    };

    while (*ptr) {
        while (*ptr && std::isspace(static_cast<unsigned char>(*ptr))) ++ptr;
        if (!*ptr) break;
        if (std::isalpha(static_cast<unsigned char>(*ptr))) {
            command = *ptr;
            ++ptr;
        }
        if (command == 0) break;
        bool relative = std::islower(static_cast<unsigned char>(command));
        char cmd = static_cast<char>(std::toupper(command));

        switch (cmd) {
            case 'M': {
                float x = 0.0f, y = 0.0f;
                if (!readFloat(x) || !readFloat(y)) {
                    return result;
                }
                if (relative) {
                    current.x += x;
                    current.y += y;
                } else {
                    current = {x, y};
                }
                start = current;
                result.subpaths.emplace_back();
                activePath = &result.subpaths.back();
                activePath->push_back(current);
                command = relative ? 'l' : 'L';
                hasPrevControlC = false;
                hasPrevControlQ = false;
                break;
            }
            case 'L': {
                ensureActivePath();
                float x = 0.0f, y = 0.0f;
                if (!readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 target = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                activePath->push_back(target);
                current = target;
                hasPrevControlC = false;
                hasPrevControlQ = false;
                break;
            }
            case 'H': {
                ensureActivePath();
                float x = 0.0f;
                if (!readFloat(x)) return result;
                Vec2 target = relative ? Vec2{current.x + x, current.y} : Vec2{x, current.y};
                activePath->push_back(target);
                current = target;
                hasPrevControlC = false;
                hasPrevControlQ = false;
                break;
            }
            case 'V': {
                ensureActivePath();
                float y = 0.0f;
                if (!readFloat(y)) return result;
                Vec2 target = relative ? Vec2{current.x, current.y + y} : Vec2{current.x, y};
                activePath->push_back(target);
                current = target;
                hasPrevControlC = false;
                hasPrevControlQ = false;
                break;
            }
            case 'C': {
                ensureActivePath();
                float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f, x = 0.0f, y = 0.0f;
                if (!readFloat(x1) || !readFloat(y1) || !readFloat(x2) || !readFloat(y2) || !readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 p1 = relative ? Vec2{current.x + x1, current.y + y1} : Vec2{x1, y1};
                Vec2 p2 = relative ? Vec2{current.x + x2, current.y + y2} : Vec2{x2, y2};
                Vec2 p3 = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                AddCubic(*activePath, current, p1, p2, p3);
                current = p3;
                prevControlC = p2;
                hasPrevControlC = true;
                hasPrevControlQ = false;
                break;
            }
            case 'S': {
                ensureActivePath();
                float x2 = 0.0f, y2 = 0.0f, x = 0.0f, y = 0.0f;
                if (!readFloat(x2) || !readFloat(y2) || !readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 cp1 = current;
                if (hasPrevControlC) {
                    cp1.x = current.x * 2.0f - prevControlC.x;
                    cp1.y = current.y * 2.0f - prevControlC.y;
                }
                Vec2 cp2 = relative ? Vec2{current.x + x2, current.y + y2} : Vec2{x2, y2};
                Vec2 p3 = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                AddCubic(*activePath, current, cp1, cp2, p3);
                current = p3;
                prevControlC = cp2;
                hasPrevControlC = true;
                hasPrevControlQ = false;
                break;
            }
            case 'Q': {
                ensureActivePath();
                float x1 = 0.0f, y1 = 0.0f, x = 0.0f, y = 0.0f;
                if (!readFloat(x1) || !readFloat(y1) || !readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 p1 = relative ? Vec2{current.x + x1, current.y + y1} : Vec2{x1, y1};
                Vec2 p2 = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                AddQuadratic(*activePath, current, p1, p2);
                current = p2;
                prevControlQ = p1;
                hasPrevControlQ = true;
                hasPrevControlC = false;
                break;
            }
            case 'T': {
                ensureActivePath();
                float x = 0.0f, y = 0.0f;
                if (!readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 cp = current;
                if (hasPrevControlQ) {
                    cp.x = current.x * 2.0f - prevControlQ.x;
                    cp.y = current.y * 2.0f - prevControlQ.y;
                }
                Vec2 p2 = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                AddQuadratic(*activePath, current, cp, p2);
                current = p2;
                prevControlQ = cp;
                hasPrevControlQ = true;
                hasPrevControlC = false;
                break;
            }
            case 'Z': {
                if (activePath && !activePath->empty()) {
                    activePath->push_back(start);
                }
                current = start;
                hasPrevControlC = false;
                hasPrevControlQ = false;
                break;
            }
            case 'A': {
                // Elliptical arc is approximated by a simple line to the end point.
                ensureActivePath();
                float rx = 0.0f, ry = 0.0f, xAxis = 0.0f, largeArc = 0.0f, sweep = 0.0f, x = 0.0f, y = 0.0f;
                if (!readFloat(rx) || !readFloat(ry) || !readFloat(xAxis) || !readFloat(largeArc) || !readFloat(sweep) || !readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 target = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                activePath->push_back(target);
                current = target;
                hasPrevControlC = false;
                hasPrevControlQ = false;
                break;
            }
            default: {
                // Skip unsupported commands by advancing one token.
                float dummy = 0.0f;
                readFloat(dummy);
                break;
            }
        }
    }
    return result;
}

Color ResolveColor(const StyleProperties& props, const Color& defaultColor, bool& hasFill) {
    Color color = defaultColor;
    hasFill = true;
    if (props.fillNone) {
        hasFill = false;
        return color;
    }
    if (props.fill.has_value()) {
        color = props.fill.value();
    }
    float opacity = 1.0f;
    if (props.opacity.has_value()) {
        opacity *= props.opacity.value();
    }
    if (props.fillOpacity.has_value()) {
        opacity *= props.fillOpacity.value();
    }
    color.a *= opacity;
    color.a = std::clamp(color.a, 0.0f, 1.0f);
    return color;
}

struct SvgDocument {
    std::vector<Shape> shapes;
    int width = 0;
    int height = 0;
};

bool ParseSVG(const std::string& text, SvgDocument& outDoc) {
    size_t pos = 0;
    float svgWidth = 0.0f;
    float svgHeight = 0.0f;
    bool hasWidth = false;
    bool hasHeight = false;
    bool hasViewBox = false;
    float viewMinX = 0.0f, viewMinY = 0.0f, viewWidth = 0.0f, viewHeight = 0.0f;
    std::unordered_map<std::string, StyleProperties> classStyles;

    while (true) {
        size_t lt = text.find('<', pos);
        if (lt == std::string::npos) break;
        size_t gt = text.find('>', lt + 1);
        if (gt == std::string::npos) break;
        std::string tagContent = text.substr(lt + 1, gt - lt - 1);
        pos = gt + 1;

        if (!tagContent.empty() && tagContent[0] == '!') {
            continue; // comment or DOCTYPE
        }
        if (!tagContent.empty() && tagContent[0] == '?') {
            continue; // XML declaration
        }
        bool closing = !tagContent.empty() && tagContent[0] == '/';
        if (closing) {
            continue;
        }
        bool selfClosing = false;
        if (!tagContent.empty() && tagContent.back() == '/') {
            selfClosing = true;
            tagContent.pop_back();
        }
        tagContent = Trim(tagContent);
        if (tagContent.empty()) continue;
        size_t space = 0;
        while (space < tagContent.size() && !std::isspace(static_cast<unsigned char>(tagContent[space]))) ++space;
        std::string tagName = ToLower(tagContent.substr(0, space));
        std::string attrText = tagContent.substr(space);
        auto attrs = ParseAttributes(tagContent);

        if (tagName == "svg") {
            auto itW = attrs.find("width");
            if (itW != attrs.end()) {
                if (auto val = ParseLength(itW->second)) {
                    svgWidth = *val;
                    hasWidth = true;
                }
            }
            auto itH = attrs.find("height");
            if (itH != attrs.end()) {
                if (auto val = ParseLength(itH->second)) {
                    svgHeight = *val;
                    hasHeight = true;
                }
            }
            auto itV = attrs.find("viewbox");
            if (itV != attrs.end()) {
                std::vector<float> numbers;
                std::stringstream ss(itV->second);
                std::string item;
                while (std::getline(ss, item, ' ')) {
                    if (item.empty()) continue;
                    size_t comma = item.find(',');
                    if (comma != std::string::npos) {
                        std::string first = item.substr(0, comma);
                        std::string second = item.substr(comma + 1);
                        float f = 0.0f;
                        if (ParseFloat(first, f)) numbers.push_back(f);
                        if (ParseFloat(second, f)) numbers.push_back(f);
                    } else {
                        float f = 0.0f;
                        if (ParseFloat(item, f)) numbers.push_back(f);
                    }
                }
                if (numbers.size() >= 4) {
                    viewMinX = numbers[0];
                    viewMinY = numbers[1];
                    viewWidth = numbers[2];
                    viewHeight = numbers[3];
                    hasViewBox = true;
                }
            }
        } else if (tagName == "style") {
            size_t close = text.find("</style>", pos);
            size_t contentEnd = (close == std::string::npos) ? text.size() : close;
            std::string styleContent = text.substr(gt + 1, contentEnd - (gt + 1));
            pos = (close == std::string::npos) ? contentEnd : close + 8;
            std::string lowered = ToLower(styleContent);
            size_t dot = 0;
            while ((dot = lowered.find('.', dot)) != std::string::npos) {
                size_t startName = dot + 1;
                size_t endName = startName;
                while (endName < lowered.size() && (std::isalnum(static_cast<unsigned char>(lowered[endName])) || lowered[endName] == '-' || lowered[endName] == '_')) {
                    ++endName;
                }
                std::string className = lowered.substr(startName, endName - startName);
                size_t braceOpen = lowered.find('{', endName);
                if (braceOpen == std::string::npos) break;
                size_t braceClose = lowered.find('}', braceOpen);
                if (braceClose == std::string::npos) break;
                std::string declarations = styleContent.substr(braceOpen + 1, braceClose - braceOpen - 1);
                StyleProperties props = ParseStyleDeclarations(declarations);
                classStyles[className].Apply(props);
                dot = braceClose + 1;
            }
        } else if (tagName == "rect" || tagName == "circle" || tagName == "ellipse" || tagName == "polygon" || tagName == "polyline" || tagName == "path" || tagName == "line") {
            StyleProperties combined;
            auto itClass = attrs.find("class");
            if (itClass != attrs.end()) {
                std::stringstream ss(itClass->second);
                std::string cls;
                while (ss >> cls) {
                    std::string key = ToLower(cls);
                    auto it = classStyles.find(key);
                    if (it != classStyles.end()) {
                        combined.Apply(it->second);
                    }
                }
            }
            auto itStyle = attrs.find("style");
            if (itStyle != attrs.end()) {
                StyleProperties inlineStyle = ParseStyleDeclarations(itStyle->second);
                combined.Apply(inlineStyle);
            }
            auto itFill = attrs.find("fill");
            if (itFill != attrs.end()) {
                Color c;
                if (ParseColorString(itFill->second, c)) {
                    StyleProperties fillProp;
                    fillProp.fill = c;
                    combined.Apply(fillProp);
                } else {
                    StyleProperties fillProp;
                    fillProp.fillNone = true;
                    combined.Apply(fillProp);
                }
            }
            auto itFillOpacity = attrs.find("fill-opacity");
            if (itFillOpacity != attrs.end()) {
                float v = 0.0f;
                if (ParseFloat(itFillOpacity->second, v)) {
                    StyleProperties fillProp;
                    fillProp.fillOpacity = std::clamp(v, 0.0f, 1.0f);
                    combined.Apply(fillProp);
                }
            }
            auto itOpacity = attrs.find("opacity");
            if (itOpacity != attrs.end()) {
                float v = 0.0f;
                if (ParseFloat(itOpacity->second, v)) {
                    StyleProperties fillProp;
                    fillProp.opacity = std::clamp(v, 0.0f, 1.0f);
                    combined.Apply(fillProp);
                }
            }

            bool hasFill = true;
            Color color = ResolveColor(combined, {1.0f, 1.0f, 1.0f, 1.0f}, hasFill);
            if (!hasFill || color.a <= 0.0f) {
                continue;
            }

            Shape shape;
            shape.color = color;

            if (tagName == "rect") {
                float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
                if (auto it = attrs.find("x"); it != attrs.end()) ParseFloat(it->second, x);
                if (auto it = attrs.find("y"); it != attrs.end()) ParseFloat(it->second, y);
                if (auto it = attrs.find("width"); it != attrs.end()) ParseFloat(it->second, w);
                if (auto it = attrs.find("height"); it != attrs.end()) ParseFloat(it->second, h);
                if (w <= 0.0f || h <= 0.0f) continue;
                shape.subpaths.push_back(BuildRectangle(x, y, w, h));
            } else if (tagName == "circle") {
                float cx = 0.0f, cy = 0.0f, r = 0.0f;
                if (auto it = attrs.find("cx"); it != attrs.end()) ParseFloat(it->second, cx);
                if (auto it = attrs.find("cy"); it != attrs.end()) ParseFloat(it->second, cy);
                if (auto it = attrs.find("r"); it != attrs.end()) ParseFloat(it->second, r);
                if (r <= 0.0f) continue;
                shape.subpaths.push_back(BuildEllipse(cx, cy, r, r));
            } else if (tagName == "ellipse") {
                float cx = 0.0f, cy = 0.0f, rx = 0.0f, ry = 0.0f;
                if (auto it = attrs.find("cx"); it != attrs.end()) ParseFloat(it->second, cx);
                if (auto it = attrs.find("cy"); it != attrs.end()) ParseFloat(it->second, cy);
                if (auto it = attrs.find("rx"); it != attrs.end()) ParseFloat(it->second, rx);
                if (auto it = attrs.find("ry"); it != attrs.end()) ParseFloat(it->second, ry);
                if (rx <= 0.0f || ry <= 0.0f) continue;
                shape.subpaths.push_back(BuildEllipse(cx, cy, rx, ry));
            } else if (tagName == "polygon" || tagName == "polyline") {
                auto itPoints = attrs.find("points");
                if (itPoints == attrs.end()) continue;
                auto pts = ParsePointList(itPoints->second);
                if (pts.size() < 3) continue;
                if (tagName == "polygon" && (pts.front().x != pts.back().x || pts.front().y != pts.back().y)) {
                    pts.push_back(pts.front());
                }
                shape.subpaths.push_back(std::move(pts));
            } else if (tagName == "line") {
                float x1 = 0.0f, y1 = 0.0f, x2 = 0.0f, y2 = 0.0f;
                if (auto it = attrs.find("x1"); it != attrs.end()) ParseFloat(it->second, x1);
                if (auto it = attrs.find("y1"); it != attrs.end()) ParseFloat(it->second, y1);
                if (auto it = attrs.find("x2"); it != attrs.end()) ParseFloat(it->second, x2);
                if (auto it = attrs.find("y2"); it != attrs.end()) ParseFloat(it->second, y2);
                std::vector<Vec2> pts = {{x1, y1}, {x2, y2}};
                shape.subpaths.push_back(std::move(pts));
            } else if (tagName == "path") {
                auto itD = attrs.find("d");
                if (itD == attrs.end()) continue;
                PathParseResult parsed = ParsePath(itD->second);
                if (parsed.subpaths.empty()) continue;
                shape.subpaths = std::move(parsed.subpaths);
            }

            if (!shape.subpaths.empty()) {
                outDoc.shapes.push_back(std::move(shape));
            }
        }

        if (!selfClosing && tagName == "style") {
            continue;
        }
    }

    if (!hasViewBox) {
        viewMinX = 0.0f;
        viewMinY = 0.0f;
        viewWidth = hasWidth ? svgWidth : 0.0f;
        viewHeight = hasHeight ? svgHeight : 0.0f;
    }
    if (!hasWidth && viewWidth > 0.0f) {
        svgWidth = viewWidth;
        hasWidth = true;
    }
    if (!hasHeight && viewHeight > 0.0f) {
        svgHeight = viewHeight;
        hasHeight = true;
    }
    if (!hasWidth || !hasHeight || svgWidth <= 0.0f || svgHeight <= 0.0f) {
        return false;
    }

    outDoc.width = std::max(1, static_cast<int>(std::round(svgWidth)));
    outDoc.height = std::max(1, static_cast<int>(std::round(svgHeight)));

    float scaleX = 1.0f;
    float scaleY = 1.0f;
    if (viewWidth > 0.0f) scaleX = svgWidth / viewWidth;
    if (viewHeight > 0.0f) scaleY = svgHeight / viewHeight;

    for (auto& shape : outDoc.shapes) {
        for (auto& path : shape.subpaths) {
            for (auto& pt : path) {
                pt.x = (pt.x - viewMinX) * scaleX;
                pt.y = (pt.y - viewMinY) * scaleY;
            }
        }
    }

    return true;
}

SDL_Surface* CreateSurface(int width, int height) {
#if SDL_MAJOR_VERSION >= 3
    return SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
#else
    return SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
#endif
}

void BlendPixel(Uint8* pixel, const Color& color) {
    float srcR = std::clamp(color.r, 0.0f, 1.0f);
    float srcG = std::clamp(color.g, 0.0f, 1.0f);
    float srcB = std::clamp(color.b, 0.0f, 1.0f);
    float srcA = std::clamp(color.a, 0.0f, 1.0f);
    float dstR = pixel[0] / 255.0f;
    float dstG = pixel[1] / 255.0f;
    float dstB = pixel[2] / 255.0f;
    float dstA = pixel[3] / 255.0f;

    float outA = srcA + dstA * (1.0f - srcA);
    if (outA < 1e-6f) {
        pixel[0] = pixel[1] = pixel[2] = pixel[3] = 0;
        return;
    }
    float outR = (srcR * srcA + dstR * dstA * (1.0f - srcA)) / outA;
    float outG = (srcG * srcA + dstG * dstA * (1.0f - srcA)) / outA;
    float outB = (srcB * srcA + dstB * dstA * (1.0f - srcA)) / outA;

    pixel[0] = static_cast<Uint8>(std::round(std::clamp(outR, 0.0f, 1.0f) * 255.0f));
    pixel[1] = static_cast<Uint8>(std::round(std::clamp(outG, 0.0f, 1.0f) * 255.0f));
    pixel[2] = static_cast<Uint8>(std::round(std::clamp(outB, 0.0f, 1.0f) * 255.0f));
    pixel[3] = static_cast<Uint8>(std::round(std::clamp(outA, 0.0f, 1.0f) * 255.0f));
}

void RasterizeShape(const Shape& shape, Uint8* pixels, int pitch, int width, int height) {
    if (shape.subpaths.empty()) return;
    Color color = shape.color;
    for (int y = 0; y < height; ++y) {
        float scanY = static_cast<float>(y) + 0.5f;
        std::vector<float> intersections;
        for (const auto& path : shape.subpaths) {
            if (path.size() < 2) continue;
            size_t count = path.size();
            for (size_t i = 0; i < count; ++i) {
                const Vec2& p1 = path[i];
                const Vec2& p2 = path[(i + 1) % count];
                if (p1.x == p2.x && p1.y == p2.y) continue;
                if (p1.y == p2.y) continue;
                float ymin = std::min(p1.y, p2.y);
                float ymax = std::max(p1.y, p2.y);
                if (scanY < ymin || scanY >= ymax) continue;
                float t = (scanY - p1.y) / (p2.y - p1.y);
                float x = p1.x + t * (p2.x - p1.x);
                intersections.push_back(x);
            }
        }
        if (intersections.empty()) continue;
        std::sort(intersections.begin(), intersections.end());
        for (size_t i = 0; i + 1 < intersections.size(); i += 2) {
            float x0 = intersections[i];
            float x1 = intersections[i + 1];
            if (x0 > x1) std::swap(x0, x1);
            int startX = std::max(0, static_cast<int>(std::floor(x0)));
            int endX = std::min(width - 1, static_cast<int>(std::ceil(x1)));
            for (int x = startX; x <= endX; ++x) {
                Uint8* pixel = pixels + y * pitch + x * 4;
                BlendPixel(pixel, color);
            }
        }
    }
}

} // namespace

SDL_Surface* LoadSVGSurface(const std::string& path, SvgRasterizationOptions options) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return nullptr;
    }
    std::string contents;
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (fileSize <= 0) {
        return nullptr;
    }
    contents.resize(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) {
        return nullptr;
    }

    SvgDocument doc;
    if (!ParseSVG(contents, doc)) {
        return nullptr;
    }

    if (doc.width <= 0 || doc.height <= 0) {
        return nullptr;
    }

    const int originalWidth = doc.width;
    const int originalHeight = doc.height;

    int requestedWidth = options.targetWidth;
    int requestedHeight = options.targetHeight;
    if (requestedWidth < 0) requestedWidth = 0;
    if (requestedHeight < 0) requestedHeight = 0;

    float scaleX = (options.scale > 0.0f) ? options.scale : 1.0f;
    float scaleY = scaleX;
    bool widthSpecified = requestedWidth > 0;
    bool heightSpecified = requestedHeight > 0;

    if (widthSpecified && heightSpecified) {
        float targetScaleX = static_cast<float>(requestedWidth) /
                             static_cast<float>(originalWidth);
        float targetScaleY = static_cast<float>(requestedHeight) /
                             static_cast<float>(originalHeight);
        if (options.preserveAspectRatio) {
            float uniform = std::min(targetScaleX, targetScaleY);
            scaleX = uniform;
            scaleY = uniform;
        } else {
            scaleX = targetScaleX;
            scaleY = targetScaleY;
        }
    } else if (widthSpecified) {
        float targetScaleX = static_cast<float>(requestedWidth) /
                             static_cast<float>(originalWidth);
        if (options.preserveAspectRatio) {
            scaleX = targetScaleX;
            scaleY = targetScaleX;
        } else {
            scaleX = targetScaleX;
        }
    } else if (heightSpecified) {
        float targetScaleY = static_cast<float>(requestedHeight) /
                             static_cast<float>(originalHeight);
        if (options.preserveAspectRatio) {
            scaleX = targetScaleY;
            scaleY = targetScaleY;
        } else {
            scaleY = targetScaleY;
        }
    }

    if (scaleX <= 0.0f) scaleX = 1.0f;
    if (scaleY <= 0.0f) scaleY = 1.0f;

    int outputWidth = std::max(1, static_cast<int>(std::round(static_cast<float>(originalWidth) * scaleX)));
    int outputHeight = std::max(1, static_cast<int>(std::round(static_cast<float>(originalHeight) * scaleY)));

    float actualScaleX = static_cast<float>(outputWidth) / static_cast<float>(originalWidth);
    float actualScaleY = static_cast<float>(outputHeight) / static_cast<float>(originalHeight);

    if (outputWidth != originalWidth || outputHeight != originalHeight) {
        for (auto& shape : doc.shapes) {
            for (auto& path : shape.subpaths) {
                for (auto& pt : path) {
                    pt.x *= actualScaleX;
                    pt.y *= actualScaleY;
                }
            }
        }
        doc.width = outputWidth;
        doc.height = outputHeight;
    }

    SDL_Surface* surface = CreateSurface(doc.width, doc.height);
    if (!surface) {
        return nullptr;
    }

    bool locked = false;
    if (SDL_MUSTLOCK(surface)) {
#if SDL_MAJOR_VERSION >= 3
        if (!SDL_LockSurface(surface)) {
            SDL_DestroySurface(surface);
            return nullptr;
        }
#else
        if (SDL_LockSurface(surface) != 0) {
            SDL_FreeSurface(surface);
            return nullptr;
        }
#endif
        locked = true;
    }

    std::fill_n(static_cast<Uint8*>(surface->pixels),
                static_cast<size_t>(surface->pitch) * static_cast<size_t>(surface->h),
                0);

    Uint8* pixels = static_cast<Uint8*>(surface->pixels);
    for (const auto& shape : doc.shapes) {
        RasterizeShape(shape, pixels, surface->pitch, surface->w, surface->h);
    }

    if (locked) {
#if SDL_MAJOR_VERSION >= 3
        SDL_UnlockSurface(surface);
#else
        SDL_UnlockSurface(surface);
#endif
    }

    return surface;
}

#else // USE_SDL
SDL_Surface* LoadSVGSurface(const std::string&, SvgRasterizationOptions) { return nullptr; }
#endif
