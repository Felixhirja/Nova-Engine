#pragma once

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

namespace simplejson {

class JsonValue;
using JsonObject = std::unordered_map<std::string, JsonValue>;
using JsonArray = std::vector<JsonValue>;

class JsonValue {
public:
    enum class Type {
        Null,
        Boolean,
        Number,
        String,
        Array,
        Object
    };

    JsonValue();
    explicit JsonValue(std::nullptr_t);
    explicit JsonValue(bool boolean);
    explicit JsonValue(double number);
    explicit JsonValue(std::string string);
    explicit JsonValue(JsonArray array);
    explicit JsonValue(JsonObject object);

    Type type() const { return type_; }

    bool IsNull() const { return type_ == Type::Null; }
    bool IsBoolean() const { return type_ == Type::Boolean; }
    bool IsNumber() const { return type_ == Type::Number; }
    bool IsString() const { return type_ == Type::String; }
    bool IsArray() const { return type_ == Type::Array; }
    bool IsObject() const { return type_ == Type::Object; }

    bool AsBoolean(bool defaultValue = false) const;
    double AsNumber(double defaultValue = 0.0) const;
    std::string AsString(const std::string& defaultValue = std::string()) const;
    const JsonArray& AsArray() const;
    const JsonObject& AsObject() const;

    JsonArray& AsArray();
    JsonObject& AsObject();

private:
    Type type_ = Type::Null;
    std::variant<std::monostate, bool, double, std::string, JsonArray, JsonObject> value_;
};

struct ParseResult {
    JsonValue value;
    bool success = false;
    std::string errorMessage;
    std::size_t errorOffset = 0;
};

ParseResult Parse(std::string_view input);

} // namespace simplejson

