#include "SimpleJson.h"

#include <charconv>
#include <cctype>
#include <stdexcept>

namespace simplejson {
namespace {

class Parser {
public:
    explicit Parser(std::string_view input) : input_(input) {}

    ParseResult ParseValue() {
        ParseResult result;
        SkipWhitespace();
        if (pos_ >= input_.size()) {
            result.errorMessage = "Unexpected end of input";
            result.errorOffset = pos_;
            return result;
        }

        char c = input_[pos_];
        if (c == 'n') {
            return ParseLiteral("null", JsonValue(nullptr));
        } else if (c == 't') {
            return ParseLiteral("true", JsonValue(true));
        } else if (c == 'f') {
            return ParseLiteral("false", JsonValue(false));
        } else if (c == '"') {
            auto stringResult = ParseString();
            if (!stringResult.success) {
                ParseResult error;
                error.errorMessage = stringResult.errorMessage;
                error.errorOffset = stringResult.errorOffset;
                return error;
            }
            ParseResult stringValue;
            stringValue.success = true;
            stringValue.value = JsonValue(std::move(stringResult.collectedString));
            stringValue.errorOffset = stringResult.errorOffset;
            return stringValue;
        } else if (c == '{') {
            return ParseObject();
        } else if (c == '[') {
            return ParseArray();
        }

        return ParseNumber();
    }

private:
    struct StringParseResult {
        bool success = false;
        std::string collectedString;
        std::string errorMessage;
        std::size_t errorOffset = 0;
    };

    ParseResult ParseLiteral(const char* literal, JsonValue value) {
        ParseResult result;
        std::size_t len = std::char_traits<char>::length(literal);
        if (input_.substr(pos_, len) == literal) {
            pos_ += len;
            result.success = true;
            result.value = std::move(value);
            result.errorOffset = pos_;
        } else {
            result.errorMessage = "Invalid literal";
            result.errorOffset = pos_;
        }
        return result;
    }

    StringParseResult ParseString() {
        StringParseResult result;
        if (pos_ >= input_.size() || input_[pos_] != '"') {
            result.errorMessage = "Expected '\"'";
            result.errorOffset = pos_;
            return result;
        }
        ++pos_;
        std::string out;
        while (pos_ < input_.size()) {
            char c = input_[pos_++];
            if (c == '"') {
                result.success = true;
                result.collectedString = std::move(out);
                result.errorOffset = pos_;
                return result;
            }
            if (c == '\\') {
                if (pos_ >= input_.size()) {
                    result.errorMessage = "Unterminated escape sequence";
                    result.errorOffset = pos_;
                    return result;
                }
                char esc = input_[pos_++];
                switch (esc) {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case 'n': out.push_back('\n'); break;
                    case 'r': out.push_back('\r'); break;
                    case 't': out.push_back('\t'); break;
                    case 'u': {
                        // Minimal unicode handling: accept 4 hex digits and skip
                        if (pos_ + 4 > input_.size()) {
                            result.errorMessage = "Invalid unicode escape";
                            result.errorOffset = pos_;
                            return result;
                        }
                        unsigned int codepoint = 0;
                        for (int i = 0; i < 4; ++i) {
                            char h = input_[pos_++];
                            codepoint <<= 4;
                            if (h >= '0' && h <= '9') codepoint += static_cast<unsigned int>(h - '0');
                            else if (h >= 'a' && h <= 'f') codepoint += static_cast<unsigned int>(10 + h - 'a');
                            else if (h >= 'A' && h <= 'F') codepoint += static_cast<unsigned int>(10 + h - 'A');
                            else {
                                result.errorMessage = "Invalid unicode escape";
                                result.errorOffset = pos_ - 1;
                                return result;
                            }
                        }
                        // Encode basic BMP range (no surrogate handling)
                        if (codepoint <= 0x7F) {
                            out.push_back(static_cast<char>(codepoint));
                        } else if (codepoint <= 0x7FF) {
                            out.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
                            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                        } else {
                            out.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
                            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
                            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
                        }
                        break;
                    }
                    default:
                        out.push_back(esc);
                        break;
                }
            } else {
                out.push_back(c);
            }
        }
        result.errorMessage = "Unterminated string";
        result.errorOffset = pos_;
        return result;
    }

    ParseResult ParseNumber() {
        ParseResult result;
        std::size_t start = pos_;
        if (input_[pos_] == '-') {
            ++pos_;
        }
        if (pos_ < input_.size() && input_[pos_] == '0') {
            ++pos_;
        } else {
            if (pos_ >= input_.size() || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                result.errorMessage = "Invalid number";
                result.errorOffset = pos_;
                return result;
            }
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }
        if (pos_ < input_.size() && input_[pos_] == '.') {
            ++pos_;
            if (pos_ >= input_.size() || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                result.errorMessage = "Invalid number";
                result.errorOffset = pos_;
                return result;
            }
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }
        if (pos_ < input_.size() && (input_[pos_] == 'e' || input_[pos_] == 'E')) {
            ++pos_;
            if (pos_ < input_.size() && (input_[pos_] == '+' || input_[pos_] == '-')) {
                ++pos_;
            }
            if (pos_ >= input_.size() || !std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                result.errorMessage = "Invalid exponent";
                result.errorOffset = pos_;
                return result;
            }
            while (pos_ < input_.size() && std::isdigit(static_cast<unsigned char>(input_[pos_]))) {
                ++pos_;
            }
        }

        double value = 0.0;
        auto span = input_.substr(start, pos_ - start);
        auto convResult = std::from_chars(span.data(), span.data() + span.size(), value);
        if (convResult.ec != std::errc()) {
            result.errorMessage = "Failed to parse number";
            result.errorOffset = start;
            return result;
        }
        result.success = true;
        result.value = JsonValue(value);
        result.errorOffset = pos_;
        return result;
    }

    ParseResult ParseArray() {
        ParseResult result;
        if (input_[pos_] != '[') {
            result.errorMessage = "Expected '['";
            result.errorOffset = pos_;
            return result;
        }
        ++pos_;
        JsonArray array;
        SkipWhitespace();
        if (pos_ < input_.size() && input_[pos_] == ']') {
            ++pos_;
            result.success = true;
            result.value = JsonValue(std::move(array));
            result.errorOffset = pos_;
            return result;
        }
        while (pos_ < input_.size()) {
            auto element = ParseValue();
            if (!element.success) {
                return element;
            }
            array.push_back(std::move(element.value));
            SkipWhitespace();
            if (pos_ < input_.size() && input_[pos_] == ',') {
                ++pos_;
                SkipWhitespace();
                continue;
            }
            if (pos_ < input_.size() && input_[pos_] == ']') {
                ++pos_;
                result.success = true;
                result.value = JsonValue(std::move(array));
                result.errorOffset = pos_;
                return result;
            }
            result.errorMessage = "Expected ',' or ']'";
            result.errorOffset = pos_;
            return result;
        }
        result.errorMessage = "Unterminated array";
        result.errorOffset = pos_;
        return result;
    }

    ParseResult ParseObject() {
        ParseResult result;
        if (input_[pos_] != '{') {
            result.errorMessage = "Expected '{'";
            result.errorOffset = pos_;
            return result;
        }
        ++pos_;
        JsonObject object;
        SkipWhitespace();
        if (pos_ < input_.size() && input_[pos_] == '}') {
            ++pos_;
            result.success = true;
            result.value = JsonValue(std::move(object));
            result.errorOffset = pos_;
            return result;
        }
        while (pos_ < input_.size()) {
            SkipWhitespace();
            auto keyResult = ParseString();
            if (!keyResult.success) {
                ParseResult err;
                err.errorMessage = keyResult.errorMessage;
                err.errorOffset = keyResult.errorOffset;
                return err;
            }
            SkipWhitespace();
            if (pos_ >= input_.size() || input_[pos_] != ':') {
                result.errorMessage = "Expected ':'";
                result.errorOffset = pos_;
                return result;
            }
            ++pos_;
            SkipWhitespace();
            auto valueResult = ParseValue();
            if (!valueResult.success) {
                return valueResult;
            }
            object.emplace(std::move(keyResult.collectedString), std::move(valueResult.value));
            SkipWhitespace();
            if (pos_ < input_.size() && input_[pos_] == ',') {
                ++pos_;
                SkipWhitespace();
                continue;
            }
            if (pos_ < input_.size() && input_[pos_] == '}') {
                ++pos_;
                result.success = true;
                result.value = JsonValue(std::move(object));
                result.errorOffset = pos_;
                return result;
            }
            result.errorMessage = "Expected ',' or '}'";
            result.errorOffset = pos_;
            return result;
        }
        result.errorMessage = "Unterminated object";
        result.errorOffset = pos_;
        return result;
    }

    void SkipWhitespace() {
        while (pos_ < input_.size() && std::isspace(static_cast<unsigned char>(input_[pos_]))) {
            ++pos_;
        }
    }

    std::size_t position() const { return pos_; }

    std::string_view input_;
    std::size_t pos_ = 0;
};

} // namespace

JsonValue::JsonValue() = default;
JsonValue::JsonValue(std::nullptr_t) : type_(Type::Null), value_(std::monostate{}) {}
JsonValue::JsonValue(bool boolean) : type_(Type::Boolean), value_(boolean) {}
JsonValue::JsonValue(double number) : type_(Type::Number), value_(number) {}
JsonValue::JsonValue(std::string string) : type_(Type::String), value_(std::move(string)) {}
JsonValue::JsonValue(JsonArray array) : type_(Type::Array), value_(std::move(array)) {}
JsonValue::JsonValue(JsonObject object) : type_(Type::Object), value_(std::move(object)) {}

bool JsonValue::AsBoolean(bool defaultValue) const {
    if (type_ == Type::Boolean) {
        return std::get<bool>(value_);
    }
    return defaultValue;
}

double JsonValue::AsNumber(double defaultValue) const {
    if (type_ == Type::Number) {
        return std::get<double>(value_);
    }
    return defaultValue;
}

std::string JsonValue::AsString(const std::string& defaultValue) const {
    if (type_ == Type::String) {
        return std::get<std::string>(value_);
    }
    return defaultValue;
}

const JsonArray& JsonValue::AsArray() const {
    if (type_ != Type::Array) {
        static JsonArray empty;
        return empty;
    }
    return std::get<JsonArray>(value_);
}

const JsonObject& JsonValue::AsObject() const {
    if (type_ != Type::Object) {
        static JsonObject empty;
        return empty;
    }
    return std::get<JsonObject>(value_);
}

JsonArray& JsonValue::AsArray() {
    if (type_ != Type::Array) {
        type_ = Type::Array;
        value_ = JsonArray{};
    }
    return std::get<JsonArray>(value_);
}

JsonObject& JsonValue::AsObject() {
    if (type_ != Type::Object) {
        type_ = Type::Object;
        value_ = JsonObject{};
    }
    return std::get<JsonObject>(value_);
}

ParseResult Parse(std::string_view input) {
    Parser parser(input);
    auto result = parser.ParseValue();
    if (!result.success) {
        return result;
    }
    std::size_t pos = result.errorOffset;
    while (pos < input.size() && std::isspace(static_cast<unsigned char>(input[pos]))) {
        ++pos;
    }
    if (pos != input.size()) {
        result.success = false;
        result.errorMessage = "Unexpected trailing data";
        result.errorOffset = pos;
    } else {
        result.errorOffset = pos;
    }
    return result;
}

} // namespace simplejson

