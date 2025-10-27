#include "SVGSurfaceLoader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#if defined(USE_FREETYPE)
#include <ft2build.h>
#include FT_FREETYPE_H
#endif

#if defined(USE_SDL)
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#endif

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

// Forward declarations for matrix helpers used in type declarations.
Matrix2D MatrixIdentity();
Matrix2D MatrixTranslate(float tx, float ty);
Matrix2D MatrixScale(float sx, float sy);
Matrix2D MatrixRotate(float angleDegrees);
Matrix2D MatrixSkewX(float angleDegrees);
Matrix2D MatrixSkewY(float angleDegrees);
Matrix2D MatrixMultiply(const Matrix2D& lhs, const Matrix2D& rhs);
std::optional<Matrix2D> MatrixInverse(const Matrix2D& m);
bool MatrixEqual(const Matrix2D& a, const Matrix2D& b);

bool StartsWith(const std::string& text, const std::string& prefix);
bool EndsWith(const std::string& text, const std::string& suffix);

struct Shape;
struct SvgDocument;
void ResolveGradientReferences(SvgDocument& doc);

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
    std::optional<std::string> fontFamily;
    std::optional<float> fontSize;
    std::optional<std::string> textAnchor;
    std::optional<float> letterSpacing;
    bool letterSpacingIsRelative = false;
    std::optional<float> lineHeight;
    bool lineHeightIsAbsolute = false;

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
        if (other.fontFamily.has_value()) {
            fontFamily = other.fontFamily;
        }
        if (other.fontSize.has_value()) {
            fontSize = other.fontSize;
        }
        if (other.textAnchor.has_value()) {
            textAnchor = other.textAnchor;
        }
        if (other.letterSpacing.has_value()) {
            letterSpacing = other.letterSpacing;
            letterSpacingIsRelative = other.letterSpacingIsRelative;
        }
        if (other.lineHeight.has_value()) {
            lineHeight = other.lineHeight;
            lineHeightIsAbsolute = other.lineHeightIsAbsolute;
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

enum class TextAnchor {
    Start,
    Middle,
    End
};

struct TextSpan {
    Vec2 origin;
    FillStyle fill;
    std::vector<std::string> fontFamilies;
    std::string debugFontFamily;
    std::vector<std::string> lines;
    float fontSize = 16.0f;
    float letterSpacing = 0.0f;
    float lineHeightMultiplier = 1.2f;
    std::optional<float> absoluteLineHeight;
    TextAnchor anchor = TextAnchor::Start;
    bool hasUnsupportedTransform = false;
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

std::string StripQuotes(const std::string& value) {
    if (value.size() >= 2) {
        char first = value.front();
        char last = value.back();
        if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
            return value.substr(1, value.size() - 2);
        }
    }
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

std::vector<std::string> SplitLines(const std::string& text) {
    std::vector<std::string> lines;
    std::stringstream ss(text);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    if (lines.empty()) {
        lines.push_back(text);
    }
    return lines;
}

std::string DecodeHtmlEntities(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (size_t i = 0; i < input.size();) {
        if (input[i] == '&') {
            size_t semicolon = input.find(';', i + 1);
            if (semicolon != std::string::npos) {
                std::string entity = input.substr(i + 1, semicolon - i - 1);
                std::string lowerEntity = ToLower(entity);
                if (lowerEntity == "amp") {
                    result.push_back('&');
                    i = semicolon + 1;
                    continue;
                }
                if (lowerEntity == "lt") {
                    result.push_back('<');
                    i = semicolon + 1;
                    continue;
                }
                if (lowerEntity == "gt") {
                    result.push_back('>');
                    i = semicolon + 1;
                    continue;
                }
                if (lowerEntity == "quot") {
                    result.push_back('"');
                    i = semicolon + 1;
                    continue;
                }
                if (lowerEntity == "apos") {
                    result.push_back('\'');
                    i = semicolon + 1;
                    continue;
                }
                if (!entity.empty() && entity[0] == '#') {
                    int base = 10;
                    size_t offset = 1;
                    if (entity.size() > 1 && (entity[1] == 'x' || entity[1] == 'X')) {
                        base = 16;
                        offset = 2;
                    }
                    try {
                        int codepoint = std::stoi(entity.substr(offset), nullptr, base);
                        if (codepoint > 0 && codepoint <= 0x10FFFF) {
                            if (codepoint <= 0x7F) {
                                result.push_back(static_cast<char>(codepoint));
                            } else if (codepoint <= 0x7FF) {
                                result.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
                                result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                            } else if (codepoint <= 0xFFFF) {
                                result.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
                                result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                                result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                            } else {
                                result.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
                                result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
                                result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                                result.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                            }
                        }
                        i = semicolon + 1;
                        continue;
                    } catch (...) {
                    }
                }
            }
        }
        result.push_back(input[i]);
        ++i;
    }
    return result;
}

std::string StripXmlTags(const std::string& text) {
    std::string output;
    output.reserve(text.size());
    bool inTag = false;
    for (char c : text) {
        if (c == '<') {
            inTag = true;
            continue;
        }
        if (c == '>') {
            inTag = false;
            continue;
        }
        if (!inTag) {
            output.push_back(c);
        }
    }
    return output;
}

std::vector<std::string> ParseFontFamilyList(const std::string& value) {
    std::vector<std::string> families;
    std::stringstream ss(value);
    std::string token;
    while (std::getline(ss, token, ',')) {
        std::string trimmed = Trim(token);
        if (trimmed.empty()) {
            continue;
        }
        trimmed = StripQuotes(trimmed);
        if (trimmed.empty()) {
            continue;
        }
        families.push_back(ToLower(trimmed));
    }
    return families;
}

TextAnchor ParseTextAnchorValue(const std::string& value) {
    std::string lower = ToLower(Trim(value));
    if (lower == "middle" || lower == "center") {
        return TextAnchor::Middle;
    }
    if (lower == "end" || lower == "right") {
        return TextAnchor::End;
    }
    return TextAnchor::Start;
}

std::optional<float> ParseScalarAllowUnits(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::string trimmed = Trim(token);
    std::string lower = ToLower(trimmed);
    if (EndsWith(lower, "px") || EndsWith(lower, "pt") || EndsWith(lower, "em")) {
        trimmed = trimmed.substr(0, trimmed.size() - 2);
    } else if (EndsWith(lower, "rem")) {
        trimmed = trimmed.substr(0, trimmed.size() - 3);
    }
    float value = 0.0f;
    if (!ParseFloat(trimmed, value)) return std::nullopt;
    return value;
}

struct LineHeightSpec {
    float value = 1.0f;
    bool isAbsolute = false;
};

std::optional<LineHeightSpec> ParseLineHeightValue(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::string trimmed = Trim(token);
    std::string lower = ToLower(trimmed);
    if (lower == "normal") {
        return LineHeightSpec{1.2f, false};
    }
    LineHeightSpec spec;
    if (EndsWith(lower, "px") || EndsWith(lower, "pt")) {
        trimmed = trimmed.substr(0, trimmed.size() - 2);
        spec.isAbsolute = true;
    } else if (EndsWith(lower, "em")) {
        trimmed = trimmed.substr(0, trimmed.size() - 2);
        spec.isAbsolute = false;
    } else if (!trimmed.empty() && trimmed.back() == '%') {
        trimmed.pop_back();
        spec.isAbsolute = false;
        float percent = 0.0f;
        if (!ParseFloat(trimmed, percent)) {
            return std::nullopt;
        }
        spec.value = percent / 100.0f;
        return spec;
    }
    float value = 0.0f;
    if (!ParseFloat(trimmed, value)) {
        return std::nullopt;
    }
    spec.value = value;
    return spec;
}

struct LetterSpacingSpec {
    float value = 0.0f;
    bool isRelative = false;
};

std::optional<LetterSpacingSpec> ParseLetterSpacingValue(const std::string& token) {
    if (token.empty()) return std::nullopt;
    std::string trimmed = Trim(token);
    std::string lower = ToLower(trimmed);
    if (lower == "normal") {
        return LetterSpacingSpec{0.0f, false};
    }
    LetterSpacingSpec spec;
    if (EndsWith(lower, "em")) {
        trimmed = trimmed.substr(0, trimmed.size() - 2);
        spec.isRelative = true;
    } else if (EndsWith(lower, "px") || EndsWith(lower, "pt")) {
        trimmed = trimmed.substr(0, trimmed.size() - 2);
        spec.isRelative = false;
    } else if (!trimmed.empty() && trimmed.back() == '%') {
        trimmed.pop_back();
        spec.isRelative = true;
        float percent = 0.0f;
        if (!ParseFloat(trimmed, percent)) {
            return std::nullopt;
        }
        spec.value = percent / 100.0f;
        return spec;
    }
    float value = 0.0f;
    if (!ParseFloat(trimmed, value)) {
        return std::nullopt;
    }
    spec.value = value;
    return spec;
}

std::vector<char32_t> DecodeUtf8(const std::string& text) {
    std::vector<char32_t> codepoints;
    size_t i = 0;
    while (i < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x80) {
            codepoints.push_back(static_cast<char32_t>(c));
            ++i;
        } else if ((c >> 5) == 0x6 && i + 1 < text.size()) {
            char32_t cp = ((c & 0x1F) << 6) |
                          (static_cast<unsigned char>(text[i + 1]) & 0x3F);
            codepoints.push_back(cp);
            i += 2;
        } else if ((c >> 4) == 0xE && i + 2 < text.size()) {
            char32_t cp = ((c & 0x0F) << 12) |
                          ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(text[i + 2]) & 0x3F);
            codepoints.push_back(cp);
            i += 3;
        } else if ((c >> 3) == 0x1E && i + 3 < text.size()) {
            char32_t cp = ((c & 0x07) << 18) |
                          ((static_cast<unsigned char>(text[i + 1]) & 0x3F) << 12) |
                          ((static_cast<unsigned char>(text[i + 2]) & 0x3F) << 6) |
                          (static_cast<unsigned char>(text[i + 3]) & 0x3F);
            codepoints.push_back(cp);
            i += 4;
        } else {
            // Invalid sequence; skip byte.
            ++i;
        }
    }
    return codepoints;
}

#if defined(USE_FREETYPE)

class SvgFontRegistry {
public:
    static SvgFontRegistry& Instance() {
        static SvgFontRegistry instance;
        return instance;
    }

    void EnsureManifestForSvg(const std::filesystem::path& svgPath) {
        Initialize();
        if (!initialized_) {
            return;
        }

        std::vector<std::filesystem::path> candidates;
        if (const char* env = std::getenv("NOVA_SVG_FONT_MANIFEST")) {
            candidates.emplace_back(env);
        }

        auto svgDir = svgPath.parent_path();
        if (!svgDir.empty()) {
            candidates.push_back(svgDir / "fonts.manifest");
            candidates.push_back(svgDir / "fonts" / "fonts.manifest");
            auto parent = svgDir.parent_path();
            if (!parent.empty()) {
                candidates.push_back(parent / "fonts.manifest");
                candidates.push_back(parent / "fonts" / "fonts.manifest");
            }
        }

        auto climb = svgDir;
        for (int i = 0; i < 4 && !climb.empty(); ++i) {
            candidates.push_back(climb / "fonts.manifest");
            candidates.push_back(climb / "fonts" / "fonts.manifest");
            climb = climb.parent_path();
        }

        for (auto& candidate : candidates) {
            std::error_code ec;
            std::filesystem::path resolved = std::filesystem::weakly_canonical(candidate, ec);
            if (ec) {
                resolved = candidate;
            }
            if (std::filesystem::exists(resolved)) {
                LoadManifest(resolved);
            }
        }
    }

    struct FontResource {
        std::filesystem::path path;
        FT_Face face = nullptr;
        bool loadAttempted = false;
    };

    const FontResource* ResolveFont(const std::vector<std::string>& families) {
        Initialize();
        if (!initialized_) {
            return nullptr;
        }
        for (const auto& fam : families) {
            std::string key = ToLower(Trim(fam));
            auto it = fonts_.find(key);
            if (it != fonts_.end()) {
                if (EnsureFaceLoaded(it->second)) {
                    return &it->second;
                }
            }
        }
        if (!defaultFamilyKey_.empty()) {
            auto it = fonts_.find(defaultFamilyKey_);
            if (it != fonts_.end() && EnsureFaceLoaded(it->second)) {
                return &it->second;
            }
        }
        for (auto& entry : fonts_) {
            if (EnsureFaceLoaded(entry.second)) {
                return &entry.second;
            }
        }
        return nullptr;
    }

    bool HasRegisteredFonts() const {
        return !fonts_.empty();
    }

private:
    SvgFontRegistry() = default;
    ~SvgFontRegistry() {
        for (auto& entry : fonts_) {
            if (entry.second.face) {
                FT_Done_Face(entry.second.face);
                entry.second.face = nullptr;
            }
        }
        if (library_) {
            FT_Done_FreeType(library_);
            library_ = nullptr;
        }
    }

    SvgFontRegistry(const SvgFontRegistry&) = delete;
    SvgFontRegistry& operator=(const SvgFontRegistry&) = delete;

    void Initialize() {
        if (initialized_) {
            return;
        }
        FT_Error err = FT_Init_FreeType(&library_);
        if (err != 0) {
            library_ = nullptr;
            std::cerr << "SVGSurfaceLoader: FreeType initialization failed (error " << err << ")" << std::endl;
        } else {
            initialized_ = true;
        }
    }

    void LoadManifest(const std::filesystem::path& manifestPath) {
        std::error_code ec;
        std::filesystem::path canonical = std::filesystem::weakly_canonical(manifestPath, ec);
        if (ec) {
            canonical = manifestPath;
        }
        std::string key = ToLower(canonical.generic_string());
        if (!loadedManifestKeys_.insert(key).second) {
            return;
        }

        std::ifstream file(manifestPath, std::ios::in);
        if (!file) {
            std::cerr << "SVGSurfaceLoader: unable to read font manifest " << manifestPath << std::endl;
            return;
        }

        std::string line;
        size_t lineNumber = 0;
        while (std::getline(file, line)) {
            ++lineNumber;
            std::string trimmed = Trim(line);
            if (trimmed.empty() || trimmed[0] == '#') {
                continue;
            }
            size_t delim = trimmed.find('=');
            if (delim == std::string::npos) {
                delim = trimmed.find(':');
            }
            if (delim == std::string::npos) {
                std::cerr << "SVGSurfaceLoader: font manifest " << manifestPath << " line " << lineNumber << " missing key/value separator" << std::endl;
                continue;
            }
            std::string left = Trim(trimmed.substr(0, delim));
            std::string right = Trim(trimmed.substr(delim + 1));
            if (left.empty() || right.empty()) {
                continue;
            }

            std::string lowerLeft = ToLower(left);
            if (lowerLeft == "default") {
                defaultFamilyKey_ = ToLower(StripQuotes(right));
                continue;
            }

            std::vector<std::string> aliases = ParseFontFamilyList(left);
            if (aliases.empty()) {
                aliases.push_back(ToLower(StripQuotes(left)));
            }

            std::string pathToken = StripQuotes(right);
            std::filesystem::path fontPath = manifestPath.parent_path() / pathToken;
            std::error_code fontEc;
            std::filesystem::path normalized = std::filesystem::weakly_canonical(fontPath, fontEc);
            if (!fontEc) {
                fontPath = normalized;
            } else {
                fontPath = fontPath.lexically_normal();
            }

            if (!std::filesystem::exists(fontPath)) {
                std::cerr << "SVGSurfaceLoader: font file not found: " << fontPath << std::endl;
            }

            for (const auto& aliasRaw : aliases) {
                std::string alias = ToLower(aliasRaw);
                FontResource& resource = fonts_[alias];
                resource.path = fontPath;
                resource.loadAttempted = false;
            }

            if (defaultFamilyKey_.empty() && !aliases.empty()) {
                defaultFamilyKey_ = ToLower(aliases.front());
            }
        }
    }

    bool EnsureFaceLoaded(FontResource& resource) {
        if (resource.face) {
            return true;
        }
        if (!initialized_ || resource.loadAttempted) {
            return false;
        }
        resource.loadAttempted = true;
        std::string pathStr = resource.path.u8string();
        FT_Face face = nullptr;
        FT_Error err = FT_New_Face(library_, pathStr.c_str(), 0, &face);
        if (err != 0) {
            std::cerr << "SVGSurfaceLoader: failed to load font '" << resource.path << "' (error " << err << ")" << std::endl;
            return false;
        }
        resource.face = face;
        return true;
    }

    FT_Library library_ = nullptr;
    bool initialized_ = false;
    std::unordered_map<std::string, FontResource> fonts_;
    std::unordered_set<std::string> loadedManifestKeys_;
    std::string defaultFamilyKey_;
};

#endif // defined(USE_FREETYPE)

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

bool EndsWith(const std::string& text, const std::string& suffix) {
    return text.size() >= suffix.size() &&
           text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
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
        } else if (name == "font-family") {
            if (!value.empty()) {
                props.fontFamily = value;
            }
        } else if (name == "font-size") {
            if (auto size = ParseScalarAllowUnits(value)) {
                props.fontSize = *size;
            }
        } else if (name == "text-anchor") {
            if (!value.empty()) {
                props.textAnchor = value;
            }
        } else if (name == "letter-spacing") {
            if (auto spacing = ParseLetterSpacingValue(value)) {
                props.letterSpacing = spacing->value;
                props.letterSpacingIsRelative = spacing->isRelative;
            }
        } else if (name == "line-height") {
            if (auto lh = ParseLineHeightValue(value)) {
                props.lineHeight = lh->value;
                props.lineHeightIsAbsolute = lh->isAbsolute;
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
    std::vector<TextSpan> texts;
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
    std::vector<StyleProperties> styleStack;
    styleStack.push_back(StyleProperties{});

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
                if (styleStack.size() > 1) {
                    styleStack.pop_back();
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

        StyleProperties elementStyle = styleStack.back();
        if (auto itClass = attrs.find("class"); itClass != attrs.end()) {
            std::stringstream ss(itClass->second);
            std::string cls;
            while (ss >> cls) {
                std::string key = ToLower(cls);
                auto it = classStyles.find(key);
                if (it != classStyles.end()) {
                    elementStyle.Apply(it->second);
                }
            }
        }
        if (auto itStyle = attrs.find("style"); itStyle != attrs.end()) {
            StyleProperties inlineStyle = ParseStyleDeclarations(itStyle->second);
            elementStyle.Apply(inlineStyle);
        }
        if (auto itFill = attrs.find("fill"); itFill != attrs.end()) {
            StyleProperties fillProp;
            if (auto url = ParseUrlReference(itFill->second)) {
                fillProp.fillUrl = *url;
                fillProp.fillNone = false;
            } else {
                Color c;
                if (ParseColorString(itFill->second, c)) {
                    fillProp.fill = c;
                    fillProp.fillNone = false;
                    fillProp.fillUrl.reset();
                } else {
                    std::string lowered = ToLower(Trim(itFill->second));
                    if (lowered == "none") {
                        fillProp.fillNone = true;
                        fillProp.fill.reset();
                        fillProp.fillUrl.reset();
                    }
                }
            }
            elementStyle.Apply(fillProp);
        }
        if (auto itFillOpacity = attrs.find("fill-opacity"); itFillOpacity != attrs.end()) {
            float v = 0.0f;
            if (ParseFloat(itFillOpacity->second, v)) {
                StyleProperties prop;
                prop.fillOpacity = std::clamp(v, 0.0f, 1.0f);
                elementStyle.Apply(prop);
            }
        }
        if (auto itOpacity = attrs.find("opacity"); itOpacity != attrs.end()) {
            float v = 0.0f;
            if (ParseFloat(itOpacity->second, v)) {
                StyleProperties prop;
                prop.opacity = std::clamp(v, 0.0f, 1.0f);
                elementStyle.Apply(prop);
            }
        }
        if (auto itFontFamily = attrs.find("font-family"); itFontFamily != attrs.end()) {
            StyleProperties prop;
            prop.fontFamily = itFontFamily->second;
            elementStyle.Apply(prop);
        }
        if (auto itFontSize = attrs.find("font-size"); itFontSize != attrs.end()) {
            if (auto size = ParseScalarAllowUnits(itFontSize->second)) {
                StyleProperties prop;
                prop.fontSize = *size;
                elementStyle.Apply(prop);
            }
        }
        if (auto itAnchor = attrs.find("text-anchor"); itAnchor != attrs.end()) {
            StyleProperties prop;
            prop.textAnchor = itAnchor->second;
            elementStyle.Apply(prop);
        }
        if (auto itLetterSpacing = attrs.find("letter-spacing"); itLetterSpacing != attrs.end()) {
            if (auto spacing = ParseLetterSpacingValue(itLetterSpacing->second)) {
                StyleProperties prop;
                prop.letterSpacing = spacing->value;
                prop.letterSpacingIsRelative = spacing->isRelative;
                elementStyle.Apply(prop);
            }
        }
        if (auto itLineHeight = attrs.find("line-height"); itLineHeight != attrs.end()) {
            if (auto lh = ParseLineHeightValue(itLineHeight->second)) {
                StyleProperties prop;
                prop.lineHeight = lh->value;
                prop.lineHeightIsAbsolute = lh->isAbsolute;
                elementStyle.Apply(prop);
            }
        }

        if (tagName == "defs") {
            if (!selfClosing) {
                elementStack.push_back(tagName);
                ++defsDepth;
                transformStack.push_back(elementTransform);
                styleStack.push_back(elementStyle);
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
                styleStack.push_back(elementStyle);
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
                styleStack.push_back(elementStyle);
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
            styleStack.push_back(elementStyle);
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
    } else if (tagName == "text") {
            if (selfClosing) {
                continue;
            }

            auto extractFirstCoordinate = [](const std::string& value, float& out) {
                auto numbers = ParseFloatList(value);
                if (!numbers.empty()) {
                    out = numbers.front();
                    return true;
                }
                return ParseFloat(value, out);
            };

            float x = 0.0f;
            float y = 0.0f;
            if (auto it = attrs.find("x"); it != attrs.end()) {
                extractFirstCoordinate(it->second, x);
            }
            if (auto it = attrs.find("y"); it != attrs.end()) {
                extractFirstCoordinate(it->second, y);
            }

            size_t contentStart = pos;
            size_t closeTagPos = text.find("</text", pos);
            if (closeTagPos == std::string::npos) {
                closeTagPos = text.size();
            }
            std::string rawContent = text.substr(contentStart, closeTagPos - contentStart);
            pos = closeTagPos;
            std::string stripped = StripXmlTags(rawContent);
            std::string decoded = DecodeHtmlEntities(stripped);

            // Preserve intentional spacing but trim leading/trailing whitespace overall.
            std::string trimmedDecoded = Trim(decoded);
            if (trimmedDecoded.empty()) {
                continue;
            }

            TextSpan span;
            span.fill = ResolveFillStyle(elementStyle, {0.0f, 0.0f, 0.0f, 1.0f});
            if (!span.fill.hasFill || (span.fill.solidColor.a <= 0.0f && !span.fill.isGradient)) {
                continue;
            }

            span.fontSize = elementStyle.fontSize.value_or(16.0f);
            if (span.fontSize <= 0.0f) {
                span.fontSize = 16.0f;
            }

            if (elementStyle.lineHeight.has_value()) {
                if (elementStyle.lineHeightIsAbsolute) {
                    span.absoluteLineHeight = elementStyle.lineHeight.value();
                } else {
                    span.lineHeightMultiplier = elementStyle.lineHeight.value();
                }
            }

            if (elementStyle.letterSpacing.has_value()) {
                float spacing = elementStyle.letterSpacing.value();
                if (elementStyle.letterSpacingIsRelative) {
                    spacing *= span.fontSize;
                }
                span.letterSpacing = spacing;
            }

            if (elementStyle.textAnchor.has_value()) {
                span.anchor = ParseTextAnchorValue(elementStyle.textAnchor.value());
            }

            if (elementStyle.fontFamily.has_value()) {
                span.debugFontFamily = elementStyle.fontFamily.value();
                span.fontFamilies = ParseFontFamilyList(elementStyle.fontFamily.value());
            }
            if (span.fontFamilies.empty()) {
                span.fontFamilies.push_back("sans-serif");
            }

            std::vector<std::string> rawLines = SplitLines(decoded);
            span.lines.reserve(rawLines.size());
            for (auto& line : rawLines) {
                span.lines.push_back(Trim(line));
            }
            if (span.lines.empty()) {
                continue;
            }

            Vec2 origin {x, y};
            Vec2 transformedOrigin = ApplyMatrix(elementTransform, origin);
            Vec2 baseOrigin = ApplyMatrix(elementTransform, {0.0f, 0.0f});
            Vec2 basisX = ApplyMatrix(elementTransform, {1.0f, 0.0f});
            basisX.x -= baseOrigin.x;
            basisX.y -= baseOrigin.y;
            Vec2 basisY = ApplyMatrix(elementTransform, {0.0f, 1.0f});
            basisY.x -= baseOrigin.x;
            basisY.y -= baseOrigin.y;
            float scaleX = std::sqrt(basisX.x * basisX.x + basisX.y * basisX.y);
            float scaleY = std::sqrt(basisY.x * basisY.x + basisY.y * basisY.y);
            if (scaleX <= 1e-6f) scaleX = 1.0f;
            if (scaleY <= 1e-6f) scaleY = 1.0f;
            if (std::abs(basisX.y) > 1e-3f || std::abs(basisY.x) > 1e-3f) {
                span.hasUnsupportedTransform = true;
            }

            span.fontSize *= scaleY;
            if (span.absoluteLineHeight.has_value()) {
                span.absoluteLineHeight = span.absoluteLineHeight.value() * scaleY;
            }
            span.letterSpacing *= scaleX;
            span.origin = transformedOrigin;

            outDoc.texts.push_back(std::move(span));
    } else if (tagName == "rect" || tagName == "circle" || tagName == "ellipse" || tagName == "polygon" || tagName == "polyline" || tagName == "path" || tagName == "line" || tagName == "use") {
            StyleProperties combined = elementStyle;

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
                                if (auto parentInverse = MatrixInverse(transformStack.back())) {
                                    Matrix2D relativeTransform = MatrixMultiply(*parentInverse, elementTransform);
                                    useTransform = MatrixMultiply(useTransform, relativeTransform);
                                }
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
    for (auto& span : outDoc.texts) {
        span.origin.x = (span.origin.x - viewMinX) * scaleX;
        span.origin.y = (span.origin.y - viewMinY) * scaleY;
        span.fontSize *= scaleY;
        if (span.absoluteLineHeight.has_value()) {
            span.absoluteLineHeight = span.absoluteLineHeight.value() * scaleY;
        }
        span.letterSpacing *= scaleX;
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

void BlendPixel(std::uint8_t* pixel, const Color& color) {
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

    pixel[0] = static_cast<std::uint8_t>(std::round(std::clamp(outR, 0.0f, 1.0f) * 255.0f));
    pixel[1] = static_cast<std::uint8_t>(std::round(std::clamp(outG, 0.0f, 1.0f) * 255.0f));
    pixel[2] = static_cast<std::uint8_t>(std::round(std::clamp(outB, 0.0f, 1.0f) * 255.0f));
    pixel[3] = static_cast<std::uint8_t>(std::round(std::clamp(outA, 0.0f, 1.0f) * 255.0f));
}

#if defined(USE_FREETYPE)

float MeasureLineWidth(FT_Face face, const std::string& line, float letterSpacing) {
    auto codepoints = DecodeUtf8(line);
    float width = 0.0f;
    FT_UInt previous = 0;
    bool firstGlyph = true;
    for (char32_t cp : codepoints) {
        if (!firstGlyph) {
            width += letterSpacing;
        }
        FT_UInt glyphIndex = FT_Get_Char_Index(face, static_cast<FT_ULong>(cp));
        if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0) {
            previous = 0;
            firstGlyph = false;
            continue;
        }
        if (!firstGlyph && FT_HAS_KERNING(face) && previous != 0 && glyphIndex != 0) {
            FT_Vector kerning;
            if (FT_Get_Kerning(face, previous, glyphIndex, FT_KERNING_DEFAULT, &kerning) == 0) {
                width += static_cast<float>(kerning.x) / 64.0f;
            }
        }
        width += static_cast<float>(face->glyph->advance.x) / 64.0f;
        previous = glyphIndex;
        firstGlyph = false;
    }
    return width;
}

void RasterizeTextSpan(const TextSpan& span,
                       const SvgDocument& doc,
                       std::uint8_t* pixels,
                       int pitch,
                       int width,
                       int height) {
    if (span.lines.empty()) {
        return;
    }
    if (span.fontSize <= 0.0f) {
        return;
    }
    if (!span.fill.hasFill) {
        return;
    }

    static bool warnedRotation = false;
    if (span.hasUnsupportedTransform && !warnedRotation) {
        std::cerr << "SVGSurfaceLoader: text transform includes rotation/skew; rendering without rotation." << std::endl;
        warnedRotation = true;
    }

    SvgFontRegistry& registry = SvgFontRegistry::Instance();
    const SvgFontRegistry::FontResource* resource = registry.ResolveFont(span.fontFamilies);
    if (!resource || resource->face == nullptr) {
        static std::unordered_set<std::string> warnedFamilies;
        std::string displayName;
        if (!span.debugFontFamily.empty()) {
            displayName = Trim(span.debugFontFamily);
        } else if (!span.fontFamilies.empty()) {
            displayName = span.fontFamilies.front();
        } else {
            displayName = "<unspecified>";
        }
        std::string warnKey = ToLower(displayName);
        if (!warnedFamilies.count(warnKey)) {
            std::cerr << "SVGSurfaceLoader: missing font for family '" << displayName << "'; text will not render." << std::endl;
            warnedFamilies.insert(warnKey);
        }
        return;
    }

    FT_Face face = resource->face;
    int pixelSize = std::max(1, static_cast<int>(std::round(span.fontSize)));
    FT_Error err = FT_Set_Pixel_Sizes(face, 0, pixelSize);
    if (err != 0) {
        static bool warnedSize = false;
        if (!warnedSize) {
            std::cerr << "SVGSurfaceLoader: failed to set FreeType pixel size (error " << err << ")" << std::endl;
            warnedSize = true;
        }
        return;
    }

    float lineAdvance = span.absoluteLineHeight.has_value()
                            ? span.absoluteLineHeight.value()
                            : span.fontSize * span.lineHeightMultiplier;
    if (lineAdvance <= 0.0f) {
        lineAdvance = span.fontSize * 1.2f;
    }

    Color baseColor = span.fill.solidColor;
    if (span.fill.isGradient) {
        auto gradientIt = doc.gradients.find(span.fill.gradientId);
        if (gradientIt != doc.gradients.end() && !gradientIt->second.stops.empty()) {
            baseColor = gradientIt->second.stops.front().color;
        }
        baseColor.a = std::clamp(baseColor.a * span.fill.opacityScale, 0.0f, 1.0f);
    }

    float letterSpacing = span.letterSpacing;

    float baselineY = span.origin.y;
    for (std::size_t lineIndex = 0; lineIndex < span.lines.size(); ++lineIndex) {
        const std::string& line = span.lines[lineIndex];
        if (line.empty()) {
            baselineY += lineAdvance;
            continue;
        }

        float lineWidth = MeasureLineWidth(face, line, letterSpacing);
        float offsetX = 0.0f;
        switch (span.anchor) {
            case TextAnchor::Middle:
                offsetX = -lineWidth * 0.5f;
                break;
            case TextAnchor::End:
                offsetX = -lineWidth;
                break;
            case TextAnchor::Start:
            default:
                offsetX = 0.0f;
                break;
        }

        float penX = span.origin.x + offsetX;
        float penY = baselineY;
        auto codepoints = DecodeUtf8(line);
        FT_UInt previous = 0;
        bool firstGlyph = true;

        for (char32_t cp : codepoints) {
            if (!firstGlyph) {
                penX += letterSpacing;
            }
            FT_UInt glyphIndex = FT_Get_Char_Index(face, static_cast<FT_ULong>(cp));
            if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT) != 0) {
                previous = 0;
                firstGlyph = false;
                continue;
            }
            if (!firstGlyph && FT_HAS_KERNING(face) && previous != 0 && glyphIndex != 0) {
                FT_Vector kerning;
                if (FT_Get_Kerning(face, previous, glyphIndex, FT_KERNING_DEFAULT, &kerning) == 0) {
                    penX += static_cast<float>(kerning.x) / 64.0f;
                }
            }
            if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER) != 0) {
                previous = 0;
                firstGlyph = false;
                continue;
            }

            FT_GlyphSlot slot = face->glyph;
            const FT_Bitmap& bitmap = slot->bitmap;
            float glyphX = penX + static_cast<float>(slot->bitmap_left);
            float glyphY = penY - static_cast<float>(slot->bitmap_top);

            for (unsigned int row = 0; row < bitmap.rows; ++row) {
                int destY = static_cast<int>(std::floor(glyphY + static_cast<float>(row)));
                if (destY < 0 || destY >= height) {
                    continue;
                }
                for (unsigned int col = 0; col < bitmap.width; ++col) {
                    int destX = static_cast<int>(std::floor(glyphX + static_cast<float>(col)));
                    if (destX < 0 || destX >= width) {
                        continue;
                    }
                    std::uint8_t coverage = bitmap.buffer[row * bitmap.pitch + col];
                    if (coverage == 0) {
                        continue;
                    }
                    float alpha = (coverage / 255.0f);
                    Color pixelColor = baseColor;
                    pixelColor.a = std::clamp(baseColor.a * alpha, 0.0f, 1.0f);
                    BlendPixel(pixels + destY * pitch + destX * 4, pixelColor);
                }
            }

            penX += static_cast<float>(slot->advance.x) / 64.0f;
            previous = glyphIndex;
            firstGlyph = false;
        }
        baselineY += lineAdvance;
    }
}

#endif // defined(USE_FREETYPE)

void RasterizeShape(const Shape& shape, const SvgDocument& doc, std::uint8_t* pixels, int pitch, int width, int height) {
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
                    std::uint8_t* pixel = pixels + y * pitch + x * 4;
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
                    std::uint8_t* pixel = pixels + y * pitch + x * 4;
                    BlendPixel(pixel, sc);
                }
            }
        }
    }
}

} // namespace

bool LoadSvgToRgba(const std::string& path,
                   std::vector<std::uint8_t>& outPixels,
                   int& outWidth,
                   int& outHeight,
                   SvgRasterizationOptions options) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }
    std::string contents;
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    if (fileSize <= 0) {
        return false;
    }
    contents.resize(static_cast<size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(contents.data(), static_cast<std::streamsize>(contents.size()));
    if (!file) {
        return false;
    }

    SvgDocument doc;
    if (!ParseSVG(contents, doc)) {
        return false;
    }

#if defined(USE_FREETYPE)
    SvgFontRegistry::Instance().EnsureManifestForSvg(std::filesystem::path(path));
#endif

    if (doc.width <= 0 || doc.height <= 0) {
        return false;
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
        for (auto& span : doc.texts) {
            span.origin.x *= actualScaleX;
            span.origin.y *= actualScaleY;
            span.fontSize *= actualScaleY;
            if (span.absoluteLineHeight.has_value()) {
                span.absoluteLineHeight = span.absoluteLineHeight.value() * actualScaleY;
            }
            span.letterSpacing *= actualScaleX;
        }
        doc.width = outputWidth;
        doc.height = outputHeight;
    }

    outWidth = doc.width;
    outHeight = doc.height;
    const std::size_t bufferSize = static_cast<std::size_t>(outWidth) * static_cast<std::size_t>(outHeight) * 4u;
    outPixels.assign(bufferSize, 0);

    std::uint8_t* pixels = outPixels.data();
    const int pitch = outWidth * 4;
    for (const auto& shape : doc.shapes) {
        RasterizeShape(shape, doc, pixels, pitch, outWidth, outHeight);
    }

#if defined(USE_FREETYPE)
    for (const auto& textSpan : doc.texts) {
        RasterizeTextSpan(textSpan, doc, pixels, pitch, outWidth, outHeight);
    }
#else
    (void)doc;
#endif

    return true;
}

#if defined(USE_SDL)

SDL_Surface* CreateSurface(int width, int height) {
#if SDL_MAJOR_VERSION >= 3
    return SDL_CreateSurface(width, height, SDL_PIXELFORMAT_RGBA32);
#else
    return SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA32);
#endif
}

SDL_Surface* LoadSVGSurface(const std::string& path, SvgRasterizationOptions options) {
    std::vector<std::uint8_t> pixels;
    int width = 0;
    int height = 0;
    if (!LoadSvgToRgba(path, pixels, width, height, options)) {
        return nullptr;
    }

    SDL_Surface* surface = CreateSurface(width, height);
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

    std::uint8_t* dst = static_cast<std::uint8_t*>(surface->pixels);
    const int pitch = surface->pitch;
    const int rowBytes = width * 4;
    if (pitch == rowBytes) {
        std::memcpy(dst, pixels.data(), static_cast<std::size_t>(rowBytes) * static_cast<std::size_t>(height));
    } else {
        for (int y = 0; y < height; ++y) {
            std::memcpy(dst + static_cast<std::size_t>(y) * static_cast<std::size_t>(pitch),
                        pixels.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(rowBytes),
                        static_cast<std::size_t>(rowBytes));
        }
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

#else

SDL_Surface* LoadSVGSurface(const std::string&, SvgRasterizationOptions) { return nullptr; }

#endif
