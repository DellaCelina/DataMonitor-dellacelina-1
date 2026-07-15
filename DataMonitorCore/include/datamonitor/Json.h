#pragma once
//
// Json.h - minimal, dependency-free JSON value type for DataMonitorCore.
//
// Supports the standard JSON scalar/composite types (Null, Bool, Number,
// String, Array, Object), a recursive-descent parser, and a serializer.
// This is the single on-disk data format used throughout DataMonitorCore.
//
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace datamonitor {

// Thrown for malformed JSON text or type-mismatched accessors.
class JsonException : public std::runtime_error {
public:
    explicit JsonException(const std::string& message) : std::runtime_error(message) {}
};

class JsonValue {
public:
    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    using Array = std::vector<JsonValue>;
    // Ordered list of key/value pairs -- preserves insertion order, unlike
    // std::map, which keeps round-tripped JSON objects looking stable.
    using Object = std::vector<std::pair<std::string, JsonValue>>;

    JsonValue() : type_(Type::Null) {}
    JsonValue(std::nullptr_t) : type_(Type::Null) {}
    JsonValue(bool value) : type_(Type::Bool), bool_(value) {}
    JsonValue(int value) : type_(Type::Number), number_(static_cast<double>(value)) {}
    JsonValue(long long value) : type_(Type::Number), number_(static_cast<double>(value)) {}
    JsonValue(std::size_t value) : type_(Type::Number), number_(static_cast<double>(value)) {}
    JsonValue(double value) : type_(Type::Number), number_(value) {}
    JsonValue(const char* value) : type_(Type::String), string_(value) {}
    JsonValue(std::string value) : type_(Type::String), string_(std::move(value)) {}
    JsonValue(Array value) : type_(Type::Array), array_(std::move(value)) {}
    JsonValue(Object value) : type_(Type::Object), object_(std::move(value)) {}

    static JsonValue MakeArray() { return JsonValue(Array{}); }
    static JsonValue MakeObject() { return JsonValue(Object{}); }

    // Parses a JSON document from text. Throws JsonException on malformed input.
    static JsonValue Parse(std::string_view text);

    // Serializes this value back to JSON text.
    std::string Dump(bool pretty = true) const;

    Type GetType() const { return type_; }
    bool IsNull() const { return type_ == Type::Null; }
    bool IsBool() const { return type_ == Type::Bool; }
    bool IsNumber() const { return type_ == Type::Number; }
    bool IsString() const { return type_ == Type::String; }
    bool IsArray() const { return type_ == Type::Array; }
    bool IsObject() const { return type_ == Type::Object; }

    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Object& AsObject() const;
    Array& AsArray();
    Object& AsObject();

    // Convenience integer accessor (truncates the underlying double).
    long long AsInt64() const { return static_cast<long long>(AsDouble()); }

    // Object member access. The non-const overload behaves like std::map::operator[]:
    // if this value is Null it becomes an Object, and a missing key is inserted
    // with a Null value. The const overload throws if the key is absent or this
    // is not an Object.
    JsonValue& operator[](const std::string& key);
    const JsonValue& operator[](const std::string& key) const;

    // Array element access.
    JsonValue& operator[](std::size_t index);
    const JsonValue& operator[](std::size_t index) const;

    bool Contains(const std::string& key) const;

    // Returns nullptr if this is not an Object or the key is not present.
    const JsonValue* Find(const std::string& key) const;
    JsonValue* Find(const std::string& key);

    // Merges the fields of `patch` (must be an Object) into this value
    // (must be an Object), overwriting fields with the same key.
    void MergeFrom(const JsonValue& patch);

    friend bool operator==(const JsonValue& lhs, const JsonValue& rhs);
    friend bool operator!=(const JsonValue& lhs, const JsonValue& rhs) { return !(lhs == rhs); }

private:
    void DumpTo(std::string& out, bool pretty, int indent) const;

    Type type_ = Type::Null;
    bool bool_ = false;
    double number_ = 0.0;
    std::string string_;
    Array array_;
    Object object_;
};

} // namespace datamonitor
