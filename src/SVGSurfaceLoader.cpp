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
#include <unordered_set>
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

struct Matrix2D {
    float a = 1.0f;
    float b = 0.0f;
    float c = 0.0f;
    float d = 1.0f;
    float e = 0.0f;
    float f = 0.0f;
};

struct GradientStop {
    float offset = 0.0f;
    Color color;
};

enum class GradientType {
    Linear,
    Radial
};

enum class GradientUnits {
    ObjectBoundingBox,
    UserSpaceOnUse
};

struct Gradient {
    GradientType type = GradientType::Linear;
    GradientUnits units = GradientUnits::ObjectBoundingBox;
    Matrix2D transform{};
    std::vector<GradientStop> stops;

    // Linear gradient parameters
    float x1 = 0.0f;
    float y1 = 0.0f;
    float x2 = 1.0f;
    float y2 = 0.0f;
    bool hasX1 = false;
    bool hasY1 = false;
    bool hasX2 = false;
    bool hasY2 = false;

    // Radial gradient parameters
    float cx = 0.5f;
    float cy = 0.5f;
    float fx = 0.5f;
    float fy = 0.5f;
    float r = 0.5f;
    bool hasCx = false;
    bool hasCy = false;
    bool hasFx = false;
    bool hasFy = false;
    bool hasR = false;

    bool hasUnits = false;
    bool hasTransform = false;
    std::optional<std::string> href;
};

struct DefinedElement {
    std::vector<Shape> shapes;
    Matrix2D transform = MatrixIdentity();
};

struct StyleProperties {
    bool fillNone = false;
    std::optional<Color> fill;
    std::optional<std::string> fillUrl;
    std::optional<float> fillOpacity;
    std::optional<float> opacity;

    void Apply(const StyleProperties& other) {
        if (other.fillNone) {
            fillNone = true;
            fill.reset();
            fillUrl.reset();
        }
        if (other.fillUrl.has_value()) {
            fillUrl = other.fillUrl;
            fill.reset();
            fillNone = false;
        }
        if (other.fill.has_value()) {
            fill = other.fill;
            fillUrl.reset();
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

struct FillStyle {
    bool hasFill = false;
    bool isGradient = false;
    Color solidColor {1.0f, 1.0f, 1.0f, 1.0f};
    std::string gradientId;
    float opacityScale = 1.0f;
};

struct Shape {
    std::vector<std::vector<Vec2>> subpaths;
    FillStyle fill;
    std::optional<Color> strokeColor;
    std::optional<float> strokeWidth;
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

std::optional<float> ParseNumberOrPercentage(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::string trimmed = Trim(token);
    bool isPercent = false;
    if (!trimmed.empty() && trimmed.back() == '%') {
        isPercent = true;
        trimmed.pop_back();
    }
    float value = 0.0f;
    if (!ParseFloat(trimmed, value)) return std::nullopt;
    if (isPercent) value /= 100.0f;
    return value;
}

std::optional<std::string> ParseUrlReference(const std::string& value) {
    std::string trimmed = Trim(value);
    if (trimmed.size() < 6) return std::nullopt;
    if (!StartsWith(ToLower(trimmed.substr(0, 4)), "url(")) return std::nullopt;
    if (trimmed.back() != ')') return std::nullopt;
    std::string inner = Trim(trimmed.substr(4, trimmed.size() - 5));
    if (!inner.empty() && inner.front() == '#') {
        inner.erase(inner.begin());
    }
    if (inner.empty()) return std::nullopt;
    return ToLower(inner);
}

std::vector<float> ParseFloatList(const std::string& text) {
    std::vector<float> values;
    const char* ptr = text.c_str();
    while (*ptr) {
        while (*ptr && (std::isspace(static_cast<unsigned char>(*ptr)) || *ptr == ',')) ++ptr;
        if (!*ptr) break;
        char* end = nullptr;
        float v = std::strtof(ptr, &end);
        if (end == ptr) break;
        values.push_back(v);
        ptr = end;
    }
    return values;
}

Matrix2D ParseTransformAttribute(const std::string& text) {
    Matrix2D result = MatrixIdentity();
    size_t pos = 0;
    const size_t len = text.size();
    while (pos < len) {
        while (pos < len && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= len) break;
        size_t startName = pos;
        while (pos < len && std::isalpha(static_cast<unsigned char>(text[pos]))) ++pos;
        if (startName == pos) break;
        std::string name = ToLower(text.substr(startName, pos - startName));
        while (pos < len && std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
        if (pos >= len || text[pos] != '(') break;
        ++pos;
        size_t argsStart = pos;
        int depth = 1;
        while (pos < len && depth > 0) {
            if (text[pos] == '(') {
                ++depth;
            } else if (text[pos] == ')') {
                --depth;
            }
            ++pos;
        }
        if (depth != 0) break;
        size_t argsEnd = pos - 1;
        std::string args = text.substr(argsStart, argsEnd - argsStart);
        auto values = ParseFloatList(args);
        Matrix2D transform = MatrixIdentity();
        if (name == "translate") {
            float tx = values.empty() ? 0.0f : values[0];
            float ty = values.size() > 1 ? values[1] : 0.0f;
            transform = MatrixTranslate(tx, ty);
        } else if (name == "scale") {
            float sx = values.empty() ? 1.0f : values[0];
            float sy = values.size() > 1 ? values[1] : sx;
            transform = MatrixScale(sx, sy);
        } else if (name == "rotate") {
            if (!values.empty()) {
                float angle = values[0];
                if (values.size() > 2) {
                    float cx = values[1];
                    float cy = values[2];
                    transform = MatrixMultiply(MatrixTranslate(cx, cy),
                                               MatrixMultiply(MatrixRotate(angle),
                                                              MatrixTranslate(-cx, -cy)));
                } else {
                    transform = MatrixRotate(angle);
                }
            }
        } else if (name == "skewx") {
            if (!values.empty()) {
                transform = MatrixSkewX(values[0]);
            }
        } else if (name == "skewy") {
            if (!values.empty()) {
                transform = MatrixSkewY(values[0]);
            }
        } else if (name == "matrix" && values.size() == 6) {
            transform.a = values[0];
            transform.b = values[1];
            transform.c = values[2];
            transform.d = values[3];
            transform.e = values[4];
            transform.f = values[5];
        }
        result = MatrixMultiply(transform, result);
        while (pos < len && (std::isspace(static_cast<unsigned char>(text[pos])) || text[pos] == ',')) ++pos;
    }
    return result;
}

Color SampleGradientStops(const std::vector<GradientStop>& stops, float t) {
    if (stops.empty()) {
        return {0.0f, 0.0f, 0.0f, 0.0f};
    }
    if (t <= stops.front().offset) {
        return stops.front().color;
    }
    if (t >= stops.back().offset) {
        return stops.back().color;
    }
    for (size_t i = 0; i + 1 < stops.size(); ++i) {
        const GradientStop& a = stops[i];
        const GradientStop& b = stops[i + 1];
        if (t >= a.offset && t <= b.offset) {
            float span = b.offset - a.offset;
            float local = (span <= 1e-6f) ? 0.0f : (t - a.offset) / span;
            Color result;
            result.r = a.color.r + (b.color.r - a.color.r) * local;
            result.g = a.color.g + (b.color.g - a.color.g) * local;
            result.b = a.color.b + (b.color.b - a.color.b) * local;
            result.a = a.color.a + (b.color.a - a.color.a) * local;
            return result;
        }
    }
    return stops.back().color;
}

bool StartsWith(const std::string& text, const std::string& prefix) {
    return text.size() >= prefix.size() && text.compare(0, prefix.size(), prefix) == 0;
}

Matrix2D MatrixIdentity() {
    return {};
}

Matrix2D MatrixMultiply(const Matrix2D& lhs, const Matrix2D& rhs) {
    Matrix2D result;
    result.a = lhs.a * rhs.a + lhs.c * rhs.b;
    result.b = lhs.b * rhs.a + lhs.d * rhs.b;
    result.c = lhs.a * rhs.c + lhs.c * rhs.d;
    result.d = lhs.b * rhs.c + lhs.d * rhs.d;
    result.e = lhs.a * rhs.e + lhs.c * rhs.f + lhs.e;
    result.f = lhs.b * rhs.e + lhs.d * rhs.f + lhs.f;
    return result;
}

Vec2 ApplyMatrix(const Matrix2D& m, const Vec2& p) {
    return {m.a * p.x + m.c * p.y + m.e, m.b * p.x + m.d * p.y + m.f};
}

std::optional<Matrix2D> MatrixInverse(const Matrix2D& m) {
    float det = m.a * m.d - m.b * m.c;
    if (std::abs(det) < 1e-6f) {
        return std::nullopt;
    }
    float invDet = 1.0f / det;
    Matrix2D inv;
    inv.a = m.d * invDet;
    inv.b = -m.b * invDet;
    inv.c = -m.c * invDet;
    inv.d = m.a * invDet;
    inv.e = (m.c * m.f - m.d * m.e) * invDet;
    inv.f = (m.b * m.e - m.a * m.f) * invDet;
    return inv;
}

Matrix2D MatrixTranslate(float tx, float ty) {
    Matrix2D m;
    m.e = tx;
    m.f = ty;
    return m;
}

Matrix2D MatrixScale(float sx, float sy) {
    Matrix2D m;
    m.a = sx;
    m.d = sy;
    return m;
}

Matrix2D MatrixRotate(float angleDegrees) {
    float rad = angleDegrees * kPi / 180.0f;
    float c = std::cos(rad);
    float s = std::sin(rad);
    Matrix2D m;
    m.a = c;
    m.b = s;
    m.c = -s;
    m.d = c;
    return m;
}

Matrix2D MatrixSkewX(float angleDegrees) {
    float rad = angleDegrees * kPi / 180.0f;
    Matrix2D m;
    m.c = std::tan(rad);
    return m;
}

Matrix2D MatrixSkewY(float angleDegrees) {
    float rad = angleDegrees * kPi / 180.0f;
    Matrix2D m;
    m.b = std::tan(rad);
    return m;
}

bool MatrixEqual(const Matrix2D& a, const Matrix2D& b) {
    const float epsilon = 1e-6f;
    return std::abs(a.a - b.a) < epsilon && std::abs(a.b - b.b) < epsilon &&
           std::abs(a.c - b.c) < epsilon && std::abs(a.d - b.d) < epsilon &&
           std::abs(a.e - b.e) < epsilon && std::abs(a.f - b.f) < epsilon;
}

struct BoundingBox {
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 0.0f;
    float maxY = 0.0f;
    bool valid = false;
};

BoundingBox ComputeBoundingBox(const Shape& shape) {
    BoundingBox box;
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();
    bool found = false;
    for (const auto& path : shape.subpaths) {
        for (const auto& pt : path) {
            minX = std::min(minX, pt.x);
            minY = std::min(minY, pt.y);
            maxX = std::max(maxX, pt.x);
            maxY = std::max(maxY, pt.y);
            found = true;
        }
    }
    if (found) {
        box.minX = minX;
        box.minY = minY;
        box.maxX = maxX;
        box.maxY = maxY;
        box.valid = true;
    }
    return box;
}

void ApplyTransformToShape(Shape& shape, const Matrix2D& transform) {
    for (auto& path : shape.subpaths) {
        for (auto& pt : path) {
            pt = ApplyMatrix(transform, pt);
        }
    }
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

void ParseStopStyleDeclarations(const std::string& text, Color& color, bool& colorSpecified, float& opacity) {
    std::stringstream ss(text);
    std::string decl;
    while (std::getline(ss, decl, ';')) {
        auto colon = decl.find(':');
        if (colon == std::string::npos) continue;
        std::string name = ToLower(Trim(decl.substr(0, colon)));
        std::string value = Trim(decl.substr(colon + 1));
        if (name == "stop-color") {
            Color parsed;
            if (ParseColorString(value, parsed)) {
                color = parsed;
                colorSpecified = true;
            }
        } else if (name == "stop-opacity") {
            if (auto val = ParseNumberOrPercentage(value)) {
                opacity = std::clamp(*val, 0.0f, 1.0f);
            }
        }
    }
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
            if (auto url = ParseUrlReference(value)) {
                props.fillUrl = *url;
                props.fill.reset();
                props.fillNone = false;
            } else {
                Color c;
                if (ParseColorString(value, c)) {
                    props.fill = c;
                    props.fillNone = false;
                    props.fillUrl.reset();
                } else {
                    std::string lowered = ToLower(Trim(value));
                    if (lowered == "none") {
                        props.fillNone = true;
                        props.fill.reset();
                        props.fillUrl.reset();
                    }
                }
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

// Helper to compute squared distance from point p to segment [v,w]
float PointSegmentDistance(const Vec2& p, const Vec2& v, const Vec2& w) {
    float lx = w.x - v.x;
    float ly = w.y - v.y;
    float l2 = lx * lx + ly * ly;
    if (l2 == 0.0f) {
        float dx = p.x - v.x; float dy = p.y - v.y; return std::sqrt(dx*dx + dy*dy);
    }
    float t = ((p.x - v.x) * lx + (p.y - v.y) * ly) / l2;
    t = std::max(0.0f, std::min(1.0f, t));
    float projx = v.x + t * lx;
    float projy = v.y + t * ly;
    float dx = p.x - projx;
    float dy = p.y - projy;
    return std::sqrt(dx*dx + dy*dy);
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

void AddArc(std::vector<Vec2>& path, const Vec2& p1, const Vec2& p2, float rx, float ry, float phi, bool large_arc, bool sweep) {
    if (rx == 0.0f || ry == 0.0f) {
        path.push_back(p2);
        return;
    }
    rx = std::abs(rx);
    ry = std::abs(ry);
    Vec2 dp = {(p1.x - p2.x) / 2.0f, (p1.y - p2.y) / 2.0f};
    float cos_phi = std::cos(-phi * kPi / 180.0f);
    float sin_phi = std::sin(-phi * kPi / 180.0f);
    Vec2 dp_rot = {dp.x * cos_phi - dp.y * sin_phi, dp.x * sin_phi + dp.y * cos_phi};
    float lambda = (dp_rot.x * dp_rot.x) / (rx * rx) + (dp_rot.y * dp_rot.y) / (ry * ry);
    if (lambda > 1.0f) {
        rx *= std::sqrt(lambda);
        ry *= std::sqrt(lambda);
    }
    float sign = large_arc == sweep ? -1.0f : 1.0f;
    float discriminant = (rx * rx * ry * ry - rx * rx * dp_rot.y * dp_rot.y - ry * ry * dp_rot.x * dp_rot.x) / (rx * rx * dp_rot.y * dp_rot.y + ry * ry * dp_rot.x * dp_rot.x);
    if (discriminant < 0.0f) discriminant = 0.0f;
    float scale = sign * std::sqrt(discriminant);
    Vec2 c_rot = {scale * (rx * dp_rot.y / ry), scale * (-ry * dp_rot.x / rx)};
    Vec2 c = {c_rot.x * cos_phi + c_rot.y * -sin_phi, c_rot.x * sin_phi + c_rot.y * cos_phi};
    Vec2 center = {(p1.x + p2.x) / 2.0f + c.x, (p1.y + p2.y) / 2.0f + c.y};
    Vec2 v1 = {(dp_rot.x - c_rot.x) / rx, (dp_rot.y - c_rot.y) / ry};
    Vec2 v2 = {(-dp_rot.x - c_rot.x) / rx, (-dp_rot.y - c_rot.y) / ry};
    float theta1 = std::atan2(v1.y, v1.x);
    float theta2 = std::atan2(v2.y, v2.x);
    float delta_theta = theta2 - theta1;
    if (!sweep && delta_theta > 0) delta_theta -= 2 * kPi;
    if (sweep && delta_theta < 0) delta_theta += 2 * kPi;
    int segments = std::max(1, static_cast<int>(std::ceil(std::abs(delta_theta) / (kPi / 2.0f))));
    float dtheta = delta_theta / segments;
    for (int i = 0; i < segments; ++i) {
        float t1 = theta1 + i * dtheta;
        float t2 = theta1 + (i + 1) * dtheta;
        Vec2 p0 = {center.x + rx * std::cos(t1) * cos_phi - ry * std::sin(t1) * sin_phi,
                   center.y + rx * std::cos(t1) * sin_phi + ry * std::sin(t1) * cos_phi};
        Vec2 p3 = {center.x + rx * std::cos(t2) * cos_phi - ry * std::sin(t2) * sin_phi,
                   center.y + rx * std::cos(t2) * sin_phi + ry * std::sin(t2) * cos_phi};
        float alpha = std::sin(dtheta) * (std::sqrt(4 + 3 * std::tan(dtheta / 2) * std::tan(dtheta / 2)) - 1) / 3;
        Vec2 p1_offset = {-rx * std::sin(t1) * cos_phi - ry * std::cos(t1) * sin_phi,
                           -rx * std::sin(t1) * sin_phi + ry * std::cos(t1) * cos_phi};
        Vec2 p2_offset = {-rx * std::sin(t2) * cos_phi - ry * std::cos(t2) * sin_phi,
                           -rx * std::sin(t2) * sin_phi + ry * std::cos(t2) * cos_phi};
        Vec2 p1 = {p0.x + alpha * p1_offset.x, p0.y + alpha * p1_offset.y};
        Vec2 p2 = {p3.x - alpha * p2_offset.x, p3.y - alpha * p2_offset.y};
        AddCubic(path, p0, p1, p2, p3);
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
                ensureActivePath();
                float rx = 0.0f, ry = 0.0f, xAxis = 0.0f, largeArc = 0.0f, sweep = 0.0f, x = 0.0f, y = 0.0f;
                if (!readFloat(rx) || !readFloat(ry) || !readFloat(xAxis) || !readFloat(largeArc) || !readFloat(sweep) || !readFloat(x) || !readFloat(y)) {
                    return result;
                }
                Vec2 target = relative ? Vec2{current.x + x, current.y + y} : Vec2{x, y};
                AddArc(*activePath, current, target, rx, ry, xAxis, largeArc != 0.0f, sweep != 0.0f);
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

FillStyle ResolveFillStyle(const StyleProperties& props, const Color& defaultColor) {
    FillStyle fill;
    if (props.fillNone) {
        fill.hasFill = false;
        return fill;
    }

    fill.hasFill = true;
    fill.solidColor = defaultColor;
    if (props.fill.has_value()) {
        fill.solidColor = props.fill.value();
    }
    if (props.fillUrl.has_value()) {
        fill.isGradient = true;
        fill.gradientId = props.fillUrl.value();
    }

    float opacity = 1.0f;
    if (props.opacity.has_value()) {
        opacity *= props.opacity.value();
    }
    if (props.fillOpacity.has_value()) {
        opacity *= props.fillOpacity.value();
    }
    opacity = std::clamp(opacity, 0.0f, 1.0f);
    fill.opacityScale = opacity;

    if (!fill.isGradient) {
        fill.solidColor.a *= opacity;
        fill.solidColor.a = std::clamp(fill.solidColor.a, 0.0f, 1.0f);
    }

    return fill;
}

struct SvgDocument {
    std::vector<Shape> shapes;
    int width = 0;
    int height = 0;
    std::unordered_map<std::string, Gradient> gradients;
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
    std::unordered_map<std::string, DefinedElement> defsElements;

    struct GradientBuilder {
        std::string id;
        Gradient gradient;
    };
    std::optional<GradientBuilder> currentGradient;
    std::vector<std::string> elementStack;
    int defsDepth = 0;
    std::vector<Matrix2D> transformStack;
    transformStack.push_back(MatrixIdentity());
    std::string currentDefsId;

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
            std::string closingName = ToLower(Trim(tagContent.substr(1)));
            if (currentGradient.has_value()) {
                bool expectLinear = currentGradient->gradient.type == GradientType::Linear;
                const char* expected = expectLinear ? "lineargradient" : "radialgradient";
                if (closingName == expected) {
                    if (currentGradient->gradient.type == GradientType::Radial) {
                        if (!currentGradient->gradient.hasFx) currentGradient->gradient.fx = currentGradient->gradient.cx;
                        if (!currentGradient->gradient.hasFy) currentGradient->gradient.fy = currentGradient->gradient.cy;
                    }
                    if (!currentGradient->id.empty()) {
                        outDoc.gradients[currentGradient->id] = std::move(currentGradient->gradient);
                    }
                    currentGradient.reset();
                }
            }
            if (!elementStack.empty() && elementStack.back() == closingName) {
                if (closingName == "defs" && defsDepth > 0) {
                    --defsDepth;
                }
                if (closingName == "g" && !currentDefsId.empty()) {
                    // Check if we're closing the current defs group
                    // For simplicity, clear currentDefsId when closing any g in defs
                    if (defsDepth > 0) {
                        currentDefsId.clear();
                    }
                }
                elementStack.pop_back();
                if (transformStack.size() > 1) {
                    transformStack.pop_back();
                }
            }
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
        auto attrs = ParseAttributes(tagContent);

        Matrix2D localTransform = MatrixIdentity();
        if (auto itTransformAttr = attrs.find("transform"); itTransformAttr != attrs.end()) {
            localTransform = ParseTransformAttribute(itTransformAttr->second);
        }
        Matrix2D parentTransform = transformStack.back();
        Matrix2D elementTransform = MatrixMultiply(parentTransform, localTransform);

        if (tagName == "defs") {
            if (!selfClosing) {
                elementStack.push_back(tagName);
                ++defsDepth;
                transformStack.push_back(elementTransform);
            }
            continue;
        }

        if (tagName == "g") {
            if (defsDepth > 0 && !selfClosing) {
                // Check if this group has an id
                auto itId = attrs.find("id");
                if (itId != attrs.end()) {
                    currentDefsId = ToLower(itId->second);
                }
            }
            if (!selfClosing) {
                elementStack.push_back(tagName);
                transformStack.push_back(elementTransform);
            }
            continue;
        }

        if (tagName == "lineargradient" || tagName == "radialgradient") {
            GradientBuilder builder;
            builder.gradient.type = (tagName == "lineargradient") ? GradientType::Linear : GradientType::Radial;
            builder.gradient.units = GradientUnits::ObjectBoundingBox;
            builder.gradient.transform = MatrixIdentity();
            if (auto itId = attrs.find("id"); itId != attrs.end()) {
                builder.id = ToLower(itId->second);
            }
            if (auto itUnits = attrs.find("gradientunits"); itUnits != attrs.end()) {
                std::string unitsLower = ToLower(Trim(itUnits->second));
                if (unitsLower == "userspaceonuse") {
                    builder.gradient.units = GradientUnits::UserSpaceOnUse;
                    builder.gradient.hasUnits = true;
                } else if (unitsLower == "objectboundingbox") {
                    builder.gradient.units = GradientUnits::ObjectBoundingBox;
                    builder.gradient.hasUnits = true;
                }
            }
            if (auto itTransform = attrs.find("gradienttransform"); itTransform != attrs.end()) {
                builder.gradient.transform = ParseTransformAttribute(itTransform->second);
                builder.gradient.hasTransform = true;
            }
            auto parseCoord = [&](const char* key, float& target, bool& flag) {
                if (auto it = attrs.find(key); it != attrs.end()) {
                    if (auto val = ParseNumberOrPercentage(it->second)) {
                        target = *val;
                        flag = true;
                    }
                }
            };
            if (builder.gradient.type == GradientType::Linear) {
                parseCoord("x1", builder.gradient.x1, builder.gradient.hasX1);
                parseCoord("y1", builder.gradient.y1, builder.gradient.hasY1);
                parseCoord("x2", builder.gradient.x2, builder.gradient.hasX2);
                parseCoord("y2", builder.gradient.y2, builder.gradient.hasY2);
            } else {
                parseCoord("cx", builder.gradient.cx, builder.gradient.hasCx);
                parseCoord("cy", builder.gradient.cy, builder.gradient.hasCy);
                parseCoord("fx", builder.gradient.fx, builder.gradient.hasFx);
                parseCoord("fy", builder.gradient.fy, builder.gradient.hasFy);
                parseCoord("r", builder.gradient.r, builder.gradient.hasR);
                if (!builder.gradient.hasR) {
                    builder.gradient.r = 0.5f;
                }
            }
            auto captureHref = [&](const char* key) {
                if (auto it = attrs.find(key); it != attrs.end()) {
                    if (auto ref = ParseUrlReference(it->second)) {
                        builder.gradient.href = *ref;
                    }
                }
            };
            captureHref("href");
            captureHref("xlink:href");

            if (selfClosing) {
                if (builder.gradient.type == GradientType::Radial) {
                    if (!builder.gradient.hasFx) builder.gradient.fx = builder.gradient.cx;
                    if (!builder.gradient.hasFy) builder.gradient.fy = builder.gradient.cy;
                }
                if (!builder.id.empty()) {
                    outDoc.gradients[builder.id] = std::move(builder.gradient);
                }
            } else {
                currentGradient = std::move(builder);
                elementStack.push_back(tagName);
                transformStack.push_back(elementTransform);
            }
            continue;
        }

        if (currentGradient.has_value()) {
            if (tagName == "stop") {
                GradientStop stop;
                stop.offset = 0.0f;
                if (auto itOffset = attrs.find("offset"); itOffset != attrs.end()) {
                    if (auto val = ParseNumberOrPercentage(itOffset->second)) {
                        stop.offset = std::clamp(*val, 0.0f, 1.0f);
                    }
                }
                Color stopColor{0.0f, 0.0f, 0.0f, 1.0f};
                bool colorSpecified = false;
                if (auto itColor = attrs.find("stop-color"); itColor != attrs.end()) {
                    Color parsedColor;
                    if (ParseColorString(itColor->second, parsedColor)) {
                        stopColor = parsedColor;
                        colorSpecified = true;
                    }
                }
                float stopOpacity = 1.0f;
                if (auto itOpacity = attrs.find("stop-opacity"); itOpacity != attrs.end()) {
                    if (auto val = ParseNumberOrPercentage(itOpacity->second)) {
                        stopOpacity = std::clamp(*val, 0.0f, 1.0f);
                    }
                }
                if (auto itStyle = attrs.find("style"); itStyle != attrs.end()) {
                    ParseStopStyleDeclarations(itStyle->second, stopColor, colorSpecified, stopOpacity);
                }
                stopColor.a *= stopOpacity;
                stop.color = stopColor;
                currentGradient->gradient.stops.push_back(stop);
            }
            continue;
        }

        if (!selfClosing) {
            elementStack.push_back(tagName);
            transformStack.push_back(elementTransform);
        }

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
    } else if (tagName == "rect" || tagName == "circle" || tagName == "ellipse" || tagName == "polygon" || tagName == "polyline" || tagName == "path" || tagName == "line" || tagName == "use") {
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
                StyleProperties fillProp;
                if (auto url = ParseUrlReference(itFill->second)) {
                    fillProp.fillUrl = *url;
                    fillProp.fillNone = false;
                } else {
                    Color c;
                    if (ParseColorString(itFill->second, c)) {
                        fillProp.fill = c;
                    } else {
                        std::string lowered = ToLower(Trim(itFill->second));
                        if (lowered == "none") {
                            fillProp.fillNone = true;
                        }
                    }
                }
                combined.Apply(fillProp);
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

            FillStyle fillStyle = ResolveFillStyle(combined, {1.0f, 1.0f, 1.0f, 1.0f});
            // stroke parsing
            std::optional<Color> strokeColor;
            std::optional<float> strokeWidth;
            auto itStroke = attrs.find("stroke");
            if (itStroke != attrs.end()) {
                Color sc;
                if (ParseColorString(itStroke->second, sc)) {
                    strokeColor = sc;
                }
            }
            auto itStrokeWidth = attrs.find("stroke-width");
            if (itStrokeWidth != attrs.end()) {
                float sw = 0.0f;
                if (ParseFloat(itStrokeWidth->second, sw) && sw > 0.0f) {
                    strokeWidth = sw;
                }
            }

            if (!fillStyle.hasFill && !strokeColor.has_value()) {
                // nothing visible
                continue;
            }

            Shape shape;
            shape.fill = fillStyle;
            shape.strokeColor = strokeColor;
            shape.strokeWidth = strokeWidth;

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
            } else if (tagName == "use") {
                // Support <use href="#id" x="..." y="..." transform="..."> referencing any defined element
                std::string href;
                float ox = 0.0f, oy = 0.0f;
                if (auto it = attrs.find("href"); it != attrs.end()) href = it->second;
                if (href.empty()) if (auto it = attrs.find("xlink:href"); it != attrs.end()) href = it->second;
                if (auto it = attrs.find("x"); it != attrs.end()) ParseFloat(it->second, ox);
                if (auto it = attrs.find("y"); it != attrs.end()) ParseFloat(it->second, oy);
                if (!href.empty() && href.size() > 1 && href[0] == '#') {
                    std::string id = ToLower(href.substr(1));
                    auto dit = defsElements.find(id);
                    if (dit != defsElements.end()) {
                        const DefinedElement& def = dit->second;
                        // Clone all shapes from the defined element
                        for (const auto& srcShape : def.shapes) {
                            Shape clonedShape = srcShape; // Copy shape
                            // Apply the defined element's transform
                            if (!MatrixEqual(def.transform, MatrixIdentity())) {
                                ApplyTransformToShape(clonedShape, def.transform);
                            }
                            // Apply the <use> element's transform (x,y offset + any transform attribute)
                            Matrix2D useTransform = MatrixTranslate(ox, oy);
                            if (!MatrixEqual(elementTransform, transformStack.back())) {
                                // elementTransform includes the <use> transform, but we need to apply it relative to parent
                                Matrix2D relativeTransform = MatrixMultiply(MatrixInverse(transformStack.back()), elementTransform);
                                useTransform = MatrixMultiply(useTransform, relativeTransform);
                            }
                            if (!MatrixEqual(useTransform, MatrixIdentity())) {
                                ApplyTransformToShape(clonedShape, useTransform);
                            }
                            outDoc.shapes.push_back(std::move(clonedShape));
                        }
                    }
                }
            }

            if (!shape.subpaths.empty()) {
                if (defsDepth > 0 && !currentDefsId.empty()) {
                    // Store in current defs group/element
                    DefinedElement& def = defsElements[currentDefsId];
                    def.shapes.push_back(std::move(shape));
                    def.transform = elementTransform;
                } else if (defsDepth > 0) {
                    // Store individual element in defs
                    auto itId = attrs.find("id");
                    if (itId != attrs.end()) {
                        std::string id = ToLower(itId->second);
                        DefinedElement& def = defsElements[id];
                        def.shapes.push_back(std::move(shape));
                        def.transform = elementTransform;
                    }
                } else {
                    outDoc.shapes.push_back(std::move(shape));
                }
            }
        }

        if (!selfClosing && tagName == "style") {
            continue;
        }
    }

    if (currentGradient.has_value()) {
        if (currentGradient->gradient.type == GradientType::Radial) {
            if (!currentGradient->gradient.hasFx) currentGradient->gradient.fx = currentGradient->gradient.cx;
            if (!currentGradient->gradient.hasFy) currentGradient->gradient.fy = currentGradient->gradient.cy;
        }
        if (!currentGradient->id.empty()) {
            outDoc.gradients[currentGradient->id] = std::move(currentGradient->gradient);
        }
        currentGradient.reset();
    }

    ResolveGradientReferences(outDoc);

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

bool ResolveGradientReferencesRecursive(const std::string& id,
                                        SvgDocument& doc,
                                        std::unordered_set<std::string>& visiting) {
    auto it = doc.gradients.find(id);
    if (it == doc.gradients.end()) return false;
    Gradient& gradient = it->second;
    if (!gradient.href.has_value()) return true;
    if (visiting.count(id) != 0) return false;

    std::string refId = gradient.href.value();
    auto refIt = doc.gradients.find(refId);
    if (refIt == doc.gradients.end()) {
        gradient.href.reset();
        return false;
    }

    visiting.insert(id);
    ResolveGradientReferencesRecursive(refId, doc, visiting);
    visiting.erase(id);

    Gradient& base = refIt->second;
    if (!gradient.hasUnits) gradient.units = base.units;
    if (!gradient.hasTransform) gradient.transform = base.transform;
    if (gradient.type == base.type) {
        if (gradient.type == GradientType::Linear) {
            if (!gradient.hasX1) gradient.x1 = base.x1;
            if (!gradient.hasY1) gradient.y1 = base.y1;
            if (!gradient.hasX2) gradient.x2 = base.x2;
            if (!gradient.hasY2) gradient.y2 = base.y2;
        } else {
            if (!gradient.hasCx) gradient.cx = base.cx;
            if (!gradient.hasCy) gradient.cy = base.cy;
            if (!gradient.hasFx) gradient.fx = base.fx;
            if (!gradient.hasFy) gradient.fy = base.fy;
            if (!gradient.hasR) gradient.r = base.r;
        }
    }
    if (gradient.stops.empty()) {
        gradient.stops = base.stops;
    }
    gradient.href.reset();
    return true;
}

void ResolveGradientReferences(SvgDocument& doc) {
    std::unordered_set<std::string> visiting;
    for (auto& entry : doc.gradients) {
        ResolveGradientReferencesRecursive(entry.first, doc, visiting);
    }
    for (auto& entry : doc.gradients) {
        Gradient& gradient = entry.second;
        if (gradient.type == GradientType::Radial) {
            if (!gradient.hasFx) gradient.fx = gradient.cx;
            if (!gradient.hasFy) gradient.fy = gradient.cy;
        }
        std::sort(gradient.stops.begin(), gradient.stops.end(),
                  [](const GradientStop& a, const GradientStop& b) {
                      return a.offset < b.offset;
                  });
    }
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

void RasterizeShape(const Shape& shape, const SvgDocument& doc, Uint8* pixels, int pitch, int width, int height) {
    if (shape.subpaths.empty()) return;

    const FillStyle& fill = shape.fill;
    bool doFill = fill.hasFill;

    struct GradientContext {
        const Gradient* gradient = nullptr;
        Matrix2D invMatrix{};
        bool valid = false;
        bool isLinear = true;
        Vec2 linearStart{};
        Vec2 linearDir{};
        float linearDirLenSq = 0.0f;
        Vec2 radialCenter{};
        float radialRadius = 0.0f;
        Color fallback {0.0f, 0.0f, 0.0f, 0.0f};
    };

    GradientContext gradCtx;
    gradCtx.fallback = fill.solidColor;
    BoundingBox bbox = ComputeBoundingBox(shape);

    if (doFill && fill.isGradient) {
        auto it = doc.gradients.find(fill.gradientId);
        if (it != doc.gradients.end() && !it->second.stops.empty()) {
            const Gradient& gradient = it->second;
            gradCtx.fallback = gradient.stops.front().color;

            Matrix2D objectMatrix = MatrixIdentity();
            bool objectValid = true;
            if (gradient.units == GradientUnits::ObjectBoundingBox) {
                if (!bbox.valid) {
                    objectValid = false;
                } else {
                    float w = bbox.maxX - bbox.minX;
                    float h = bbox.maxY - bbox.minY;
                    if (w <= 1e-4f || h <= 1e-4f) {
                        objectValid = false;
                    } else {
                        objectMatrix.a = w;
                        objectMatrix.d = h;
                        objectMatrix.e = bbox.minX;
                        objectMatrix.f = bbox.minY;
                    }
                }
            }
            if (objectValid) {
                Matrix2D combined = MatrixMultiply(objectMatrix, gradient.transform);
                if (auto inv = MatrixInverse(combined)) {
                    gradCtx.gradient = &gradient;
                    gradCtx.invMatrix = *inv;
                    gradCtx.valid = true;
                    gradCtx.isLinear = (gradient.type == GradientType::Linear);
                    if (gradCtx.isLinear) {
                        gradCtx.linearStart = {gradient.x1, gradient.y1};
                        Vec2 end = {gradient.x2, gradient.y2};
                        gradCtx.linearDir = {end.x - gradCtx.linearStart.x, end.y - gradCtx.linearStart.y};
                        gradCtx.linearDirLenSq = gradCtx.linearDir.x * gradCtx.linearDir.x +
                                                 gradCtx.linearDir.y * gradCtx.linearDir.y;
                        if (gradCtx.linearDirLenSq <= 1e-8f) {
                            gradCtx.valid = false;
                        }
                    } else {
                        gradCtx.radialCenter = {gradient.cx, gradient.cy};
                        gradCtx.radialRadius = std::max(gradient.r, 1e-6f);
                    }
                }
            }
        }
    }

    auto sampleFillColor = [&](float px, float py) -> Color {
        if (!fill.isGradient) {
            return fill.solidColor;
        }
        if (!gradCtx.valid || gradCtx.gradient == nullptr) {
            Color fallback = gradCtx.fallback;
            fallback.a = std::clamp(fallback.a * fill.opacityScale, 0.0f, 1.0f);
            return fallback;
        }
        Color result;
        Vec2 point{px, py};
        Vec2 coord = ApplyMatrix(gradCtx.invMatrix, point);
        if (gradCtx.isLinear) {
            float t = 0.0f;
            Vec2 diff{coord.x - gradCtx.linearStart.x, coord.y - gradCtx.linearStart.y};
            if (gradCtx.linearDirLenSq > 1e-8f) {
                t = (diff.x * gradCtx.linearDir.x + diff.y * gradCtx.linearDir.y) / gradCtx.linearDirLenSq;
            }
            t = std::clamp(t, 0.0f, 1.0f);
            result = SampleGradientStops(gradCtx.gradient->stops, t);
        } else {
            Vec2 diff{coord.x - gradCtx.radialCenter.x, coord.y - gradCtx.radialCenter.y};
            float dist = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            float t = dist / gradCtx.radialRadius;
            t = std::clamp(t, 0.0f, 1.0f);
            result = SampleGradientStops(gradCtx.gradient->stops, t);
        }
        result.a = std::clamp(result.a * fill.opacityScale, 0.0f, 1.0f);
        return result;
    };

    if (doFill) {
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
                    float sampleX = static_cast<float>(x) + 0.5f;
                    Color color = sampleFillColor(sampleX, scanY);
                    Uint8* pixel = pixels + y * pitch + x * 4;
                    BlendPixel(pixel, color);
                }
            }
        }
    }

    if (shape.strokeColor.has_value() && shape.strokeWidth.has_value() && shape.strokeWidth.value() > 0.0f) {
        Color sc = shape.strokeColor.value();
        float halfWidth = shape.strokeWidth.value() * 0.5f;
        for (int y = 0; y < height; ++y) {
            float py = static_cast<float>(y) + 0.5f;
            for (int x = 0; x < width; ++x) {
                float px = static_cast<float>(x) + 0.5f;
                bool inStroke = false;
                for (const auto& path : shape.subpaths) {
                    if (path.size() < 2) continue;
                    for (size_t i = 0; i + 1 < path.size(); ++i) {
                        const Vec2& v = path[i];
                        const Vec2& w = path[i + 1];
                        float d = PointSegmentDistance({px, py}, v, w);
                        if (d <= halfWidth) { inStroke = true; break; }
                    }
                    if (inStroke) break;
                }
                if (inStroke) {
                    Uint8* pixel = pixels + y * pitch + x * 4;
                    BlendPixel(pixel, sc);
                }
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
        RasterizeShape(shape, doc, pixels, surface->pitch, surface->w, surface->h);
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
