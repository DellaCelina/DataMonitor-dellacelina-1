#include "datamonitor/Json.h"

#include <charconv>
#include <cmath>
#include <cstdio>

namespace datamonitor {

namespace {

// ---------------------------------------------------------------------
// Parser
// ---------------------------------------------------------------------
class Parser {
public:
    explicit Parser(std::string_view text) : text_(text) {}

    JsonValue ParseDocument() {
        SkipWhitespace();
        JsonValue value = ParseValue();
        SkipWhitespace();
        if (pos_ != text_.size()) {
            throw JsonException("Trailing characters after JSON document");
        }
        return value;
    }

private:
    std::string_view text_;
    std::size_t pos_ = 0;

    char Peek() const {
        if (pos_ >= text_.size()) {
            throw JsonException("Unexpected end of JSON input");
        }
        return text_[pos_];
    }

    bool AtEnd() const { return pos_ >= text_.size(); }

    char Advance() { return text_[pos_++]; }

    void SkipWhitespace() {
        while (!AtEnd()) {
            char c = text_[pos_];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos_;
            } else {
                break;
            }
        }
    }

    void Expect(char expected) {
        if (AtEnd() || text_[pos_] != expected) {
            throw JsonException(std::string("Expected '") + expected + "' in JSON input");
        }
        ++pos_;
    }

    bool Consume(std::string_view literal) {
        if (text_.substr(pos_).rfind(literal, 0) == 0) {
            pos_ += literal.size();
            return true;
        }
        return false;
    }

    JsonValue ParseValue() {
        SkipWhitespace();
        if (AtEnd()) {
            throw JsonException("Unexpected end of JSON input");
        }
        char c = Peek();
        switch (c) {
            case '{': return ParseObject();
            case '[': return ParseArray();
            case '"': return JsonValue(ParseString());
            case 't':
                if (Consume("true")) return JsonValue(true);
                throw JsonException("Invalid literal in JSON input");
            case 'f':
                if (Consume("false")) return JsonValue(false);
                throw JsonException("Invalid literal in JSON input");
            case 'n':
                if (Consume("null")) return JsonValue(nullptr);
                throw JsonException("Invalid literal in JSON input");
            default:
                if (c == '-' || (c >= '0' && c <= '9')) {
                    return ParseNumber();
                }
                throw JsonException("Unexpected character in JSON input");
        }
    }

    JsonValue ParseObject() {
        Expect('{');
        JsonValue::Object object;
        SkipWhitespace();
        if (!AtEnd() && Peek() == '}') {
            ++pos_;
            return JsonValue(std::move(object));
        }
        while (true) {
            SkipWhitespace();
            if (AtEnd() || Peek() != '"') {
                throw JsonException("Expected string key in JSON object");
            }
            std::string key = ParseString();
            SkipWhitespace();
            Expect(':');
            JsonValue value = ParseValue();
            object.emplace_back(std::move(key), std::move(value));
            SkipWhitespace();
            if (AtEnd()) {
                throw JsonException("Unterminated JSON object");
            }
            char c = Advance();
            if (c == ',') {
                continue;
            } else if (c == '}') {
                break;
            } else {
                throw JsonException("Expected ',' or '}' in JSON object");
            }
        }
        return JsonValue(std::move(object));
    }

    JsonValue ParseArray() {
        Expect('[');
        JsonValue::Array array;
        SkipWhitespace();
        if (!AtEnd() && Peek() == ']') {
            ++pos_;
            return JsonValue(std::move(array));
        }
        while (true) {
            JsonValue value = ParseValue();
            array.push_back(std::move(value));
            SkipWhitespace();
            if (AtEnd()) {
                throw JsonException("Unterminated JSON array");
            }
            char c = Advance();
            if (c == ',') {
                continue;
            } else if (c == ']') {
                break;
            } else {
                throw JsonException("Expected ',' or ']' in JSON array");
            }
        }
        return JsonValue(std::move(array));
    }

    static void AppendUtf8(std::string& out, unsigned int codepoint) {
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else if (codepoint <= 0xFFFF) {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
    }

    unsigned int ParseHex4() {
        if (pos_ + 4 > text_.size()) {
            throw JsonException("Invalid \\u escape in JSON string");
        }
        unsigned int value = 0;
        for (int i = 0; i < 4; ++i) {
            char c = text_[pos_++];
            value <<= 4;
            if (c >= '0' && c <= '9') value |= static_cast<unsigned int>(c - '0');
            else if (c >= 'a' && c <= 'f') value |= static_cast<unsigned int>(c - 'a' + 10);
            else if (c >= 'A' && c <= 'F') value |= static_cast<unsigned int>(c - 'A' + 10);
            else throw JsonException("Invalid hex digit in \\u escape");
        }
        return value;
    }

    std::string ParseString() {
        Expect('"');
        std::string out;
        while (true) {
            if (AtEnd()) {
                throw JsonException("Unterminated JSON string");
            }
            char c = Advance();
            if (c == '"') {
                break;
            }
            if (c == '\\') {
                if (AtEnd()) {
                    throw JsonException("Unterminated escape in JSON string");
                }
                char esc = Advance();
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
                        unsigned int codepoint = ParseHex4();
                        if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
                            // High surrogate; expect a low surrogate next.
                            if (pos_ + 2 <= text_.size() && text_[pos_] == '\\' && text_[pos_ + 1] == 'u') {
                                pos_ += 2;
                                unsigned int low = ParseHex4();
                                if (low >= 0xDC00 && low <= 0xDFFF) {
                                    codepoint = 0x10000 + ((codepoint - 0xD800) << 10) + (low - 0xDC00);
                                } else {
                                    throw JsonException("Invalid low surrogate in \\u escape");
                                }
                            } else {
                                throw JsonException("Expected low surrogate after high surrogate");
                            }
                        }
                        AppendUtf8(out, codepoint);
                        break;
                    }
                    default:
                        throw JsonException("Invalid escape character in JSON string");
                }
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    JsonValue ParseNumber() {
        std::size_t start = pos_;
        if (!AtEnd() && Peek() == '-') ++pos_;
        if (AtEnd() || !std::isdigit(static_cast<unsigned char>(Peek()))) {
            throw JsonException("Invalid number in JSON input");
        }
        if (Peek() == '0') {
            ++pos_;
        } else {
            while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) ++pos_;
        }
        if (!AtEnd() && Peek() == '.') {
            ++pos_;
            if (AtEnd() || !std::isdigit(static_cast<unsigned char>(Peek()))) {
                throw JsonException("Invalid number in JSON input");
            }
            while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) ++pos_;
        }
        if (!AtEnd() && (Peek() == 'e' || Peek() == 'E')) {
            ++pos_;
            if (!AtEnd() && (Peek() == '+' || Peek() == '-')) ++pos_;
            if (AtEnd() || !std::isdigit(static_cast<unsigned char>(Peek()))) {
                throw JsonException("Invalid number in JSON input");
            }
            while (!AtEnd() && std::isdigit(static_cast<unsigned char>(Peek()))) ++pos_;
        }
        std::string token(text_.substr(start, pos_ - start));
        double value = 0.0;
        try {
            value = std::stod(token);
        } catch (const std::exception&) {
            throw JsonException("Invalid number in JSON input: " + token);
        }
        return JsonValue(value);
    }
};

void AppendEscapedString(std::string& out, const std::string& value) {
    out.push_back('"');
    for (unsigned char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out.push_back(static_cast<char>(c));
                }
                break;
        }
    }
    out.push_back('"');
}

void AppendNumber(std::string& out, double value) {
    if (std::isfinite(value) && value == std::floor(value) &&
        std::abs(value) < 1e15) {
        // Print integral-valued doubles without a trailing ".0" so ids etc. look clean.
        long long asInt = static_cast<long long>(value);
        out += std::to_string(asInt);
    } else {
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.17g", value);
        out += buf;
    }
}

} // namespace

JsonValue JsonValue::Parse(std::string_view text) {
    Parser parser(text);
    return parser.ParseDocument();
}

bool JsonValue::AsBool() const {
    if (type_ != Type::Bool) throw JsonException("JsonValue is not a Bool");
    return bool_;
}

double JsonValue::AsDouble() const {
    if (type_ != Type::Number) throw JsonException("JsonValue is not a Number");
    return number_;
}

const std::string& JsonValue::AsString() const {
    if (type_ != Type::String) throw JsonException("JsonValue is not a String");
    return string_;
}

const JsonValue::Array& JsonValue::AsArray() const {
    if (type_ != Type::Array) throw JsonException("JsonValue is not an Array");
    return array_;
}

JsonValue::Array& JsonValue::AsArray() {
    if (type_ != Type::Array) throw JsonException("JsonValue is not an Array");
    return array_;
}

const JsonValue::Object& JsonValue::AsObject() const {
    if (type_ != Type::Object) throw JsonException("JsonValue is not an Object");
    return object_;
}

JsonValue::Object& JsonValue::AsObject() {
    if (type_ != Type::Object) throw JsonException("JsonValue is not an Object");
    return object_;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (type_ == Type::Null) {
        type_ = Type::Object;
    }
    if (type_ != Type::Object) throw JsonException("JsonValue is not an Object");
    for (auto& kv : object_) {
        if (kv.first == key) return kv.second;
    }
    object_.emplace_back(key, JsonValue());
    return object_.back().second;
}

const JsonValue& JsonValue::operator[](const std::string& key) const {
    if (type_ != Type::Object) throw JsonException("JsonValue is not an Object");
    for (auto& kv : object_) {
        if (kv.first == key) return kv.second;
    }
    throw JsonException("Key not found in JSON object: " + key);
}

JsonValue& JsonValue::operator[](std::size_t index) {
    if (type_ != Type::Array) throw JsonException("JsonValue is not an Array");
    if (index >= array_.size()) throw JsonException("Array index out of range");
    return array_[index];
}

const JsonValue& JsonValue::operator[](std::size_t index) const {
    if (type_ != Type::Array) throw JsonException("JsonValue is not an Array");
    if (index >= array_.size()) throw JsonException("Array index out of range");
    return array_[index];
}

bool JsonValue::Contains(const std::string& key) const {
    if (type_ != Type::Object) return false;
    for (auto& kv : object_) {
        if (kv.first == key) return true;
    }
    return false;
}

const JsonValue* JsonValue::Find(const std::string& key) const {
    if (type_ != Type::Object) return nullptr;
    for (auto& kv : object_) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

JsonValue* JsonValue::Find(const std::string& key) {
    if (type_ != Type::Object) return nullptr;
    for (auto& kv : object_) {
        if (kv.first == key) return &kv.second;
    }
    return nullptr;
}

void JsonValue::MergeFrom(const JsonValue& patch) {
    if (type_ == Type::Null) type_ = Type::Object;
    if (type_ != Type::Object || patch.type_ != Type::Object) {
        throw JsonException("MergeFrom requires two JSON objects");
    }
    for (const auto& kv : patch.object_) {
        (*this)[kv.first] = kv.second;
    }
}

bool operator==(const JsonValue& lhs, const JsonValue& rhs) {
    if (lhs.type_ != rhs.type_) return false;
    switch (lhs.type_) {
        case JsonValue::Type::Null: return true;
        case JsonValue::Type::Bool: return lhs.bool_ == rhs.bool_;
        case JsonValue::Type::Number: return lhs.number_ == rhs.number_;
        case JsonValue::Type::String: return lhs.string_ == rhs.string_;
        case JsonValue::Type::Array: return lhs.array_ == rhs.array_;
        case JsonValue::Type::Object: {
            if (lhs.object_.size() != rhs.object_.size()) return false;
            for (const auto& kv : lhs.object_) {
                const JsonValue* other = rhs.Find(kv.first);
                if (!other || !(*other == kv.second)) return false;
            }
            return true;
        }
    }
    return false;
}

std::string JsonValue::Dump(bool pretty) const {
    std::string out;
    DumpTo(out, pretty, 0);
    return out;
}

void JsonValue::DumpTo(std::string& out, bool pretty, int indent) const {
    auto writeIndent = [&](int level) {
        if (pretty) {
            out.push_back('\n');
            out.append(static_cast<std::size_t>(level) * 2, ' ');
        }
    };

    switch (type_) {
        case Type::Null:
            out += "null";
            break;
        case Type::Bool:
            out += bool_ ? "true" : "false";
            break;
        case Type::Number:
            AppendNumber(out, number_);
            break;
        case Type::String:
            AppendEscapedString(out, string_);
            break;
        case Type::Array: {
            if (array_.empty()) {
                out += "[]";
                break;
            }
            out.push_back('[');
            bool first = true;
            for (const auto& item : array_) {
                if (!first) out.push_back(',');
                first = false;
                writeIndent(indent + 1);
                item.DumpTo(out, pretty, indent + 1);
            }
            writeIndent(indent);
            out.push_back(']');
            break;
        }
        case Type::Object: {
            if (object_.empty()) {
                out += "{}";
                break;
            }
            out.push_back('{');
            bool first = true;
            for (const auto& kv : object_) {
                if (!first) out.push_back(',');
                first = false;
                writeIndent(indent + 1);
                AppendEscapedString(out, kv.first);
                out.push_back(':');
                if (pretty) out.push_back(' ');
                kv.second.DumpTo(out, pretty, indent + 1);
            }
            writeIndent(indent);
            out.push_back('}');
            break;
        }
    }
}

} // namespace datamonitor
