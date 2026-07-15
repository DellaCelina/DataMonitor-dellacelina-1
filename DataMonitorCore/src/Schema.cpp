#include "datamonitor/Schema.h"

#include <algorithm>
#include <sstream>

namespace datamonitor {

namespace {

std::string JoinErrors(const std::vector<std::string>& errors) {
    std::ostringstream out;
    out << errors.size() << " schema violation(s)";
    if (!errors.empty()) {
        out << ": " << errors.front();
        if (errors.size() > 1) out << " (and " << (errors.size() - 1) << " more)";
    }
    return out.str();
}

std::string DumpForMessage(const JsonValue& value) {
    return value.Dump(false);
}

} // namespace

SchemaValidationException::SchemaValidationException(std::vector<std::string> errors)
    : std::runtime_error(JoinErrors(errors)), errors_(std::move(errors)) {}

std::string FieldTypeToString(FieldType type) {
    switch (type) {
        case FieldType::String: return "string";
        case FieldType::Number: return "number";
        case FieldType::Boolean: return "boolean";
        case FieldType::Array: return "array";
        case FieldType::Object: return "object";
        case FieldType::Any: return "any";
    }
    return "any";
}

FieldType FieldTypeFromString(const std::string& text) {
    if (text == "string") return FieldType::String;
    if (text == "number") return FieldType::Number;
    if (text == "boolean") return FieldType::Boolean;
    if (text == "array") return FieldType::Array;
    if (text == "object") return FieldType::Object;
    if (text == "any") return FieldType::Any;
    throw SchemaException("Unknown field type: " + text);
}

JsonValue FieldDefinition::ToJson() const {
    JsonValue json = JsonValue::MakeObject();
    json["name"] = JsonValue(name);
    json["type"] = JsonValue(FieldTypeToString(type));
    json["required"] = JsonValue(required);
    if (minValue) json["min"] = JsonValue(*minValue);
    if (maxValue) json["max"] = JsonValue(*maxValue);
    if (minLength) json["minLength"] = JsonValue(*minLength);
    if (maxLength) json["maxLength"] = JsonValue(*maxLength);
    if (allowedValues) {
        JsonValue array = JsonValue::MakeArray();
        for (const auto& value : *allowedValues) {
            array.AsArray().push_back(value);
        }
        json["allowedValues"] = array;
    }
    return json;
}

FieldDefinition FieldDefinition::FromJson(const JsonValue& json) {
    if (!json.IsObject()) {
        throw SchemaException("Field definition must be a JSON object");
    }
    const JsonValue* nameValue = json.Find("name");
    if (!nameValue || !nameValue->IsString() || nameValue->AsString().empty()) {
        throw SchemaException("Field definition is missing a non-empty 'name'");
    }

    FieldDefinition field;
    field.name = nameValue->AsString();

    if (const JsonValue* typeValue = json.Find("type")) {
        field.type = FieldTypeFromString(typeValue->AsString());
    }
    if (const JsonValue* requiredValue = json.Find("required")) {
        field.required = requiredValue->AsBool();
    }
    if (const JsonValue* minValue = json.Find("min")) {
        field.minValue = minValue->AsDouble();
    }
    if (const JsonValue* maxValue = json.Find("max")) {
        field.maxValue = maxValue->AsDouble();
    }
    if (const JsonValue* minLengthValue = json.Find("minLength")) {
        field.minLength = static_cast<std::size_t>(minLengthValue->AsDouble());
    }
    if (const JsonValue* maxLengthValue = json.Find("maxLength")) {
        field.maxLength = static_cast<std::size_t>(maxLengthValue->AsDouble());
    }
    if (const JsonValue* allowedValues = json.Find("allowedValues")) {
        std::vector<JsonValue> values;
        for (const auto& value : allowedValues->AsArray()) {
            values.push_back(value);
        }
        field.allowedValues = std::move(values);
    }
    return field;
}

void TableSchema::AddField(FieldDefinition field) {
    if (field.name.empty()) {
        throw SchemaException("Field name must not be empty");
    }
    if (HasField(field.name)) {
        throw SchemaException("Field '" + field.name + "' already exists in this schema");
    }
    fields_.push_back(std::move(field));
}

bool TableSchema::RemoveField(const std::string& name) {
    auto it = std::find_if(fields_.begin(), fields_.end(), [&](const FieldDefinition& f) { return f.name == name; });
    if (it == fields_.end()) return false;
    fields_.erase(it);
    return true;
}

bool TableSchema::HasField(const std::string& name) const {
    return std::any_of(fields_.begin(), fields_.end(), [&](const FieldDefinition& f) { return f.name == name; });
}

std::optional<FieldDefinition> TableSchema::GetField(const std::string& name) const {
    auto it = std::find_if(fields_.begin(), fields_.end(), [&](const FieldDefinition& f) { return f.name == name; });
    if (it == fields_.end()) return std::nullopt;
    return *it;
}

std::vector<std::string> TableSchema::Validate(const JsonValue& record, const std::string& ignoredField) const {
    std::vector<std::string> errors;

    if (!record.IsObject()) {
        errors.push_back("레코드는 JSON 객체여야 합니다.");
        return errors;
    }

    for (const auto& field : fields_) {
        const JsonValue* value = record.Find(field.name);
        if (!value || value->IsNull()) {
            if (field.required) {
                errors.push_back("'" + field.name + "' 필드는 필수입니다.");
            }
            continue;
        }

        bool typeOk = true;
        switch (field.type) {
            case FieldType::String: typeOk = value->IsString(); break;
            case FieldType::Number: typeOk = value->IsNumber(); break;
            case FieldType::Boolean: typeOk = value->IsBool(); break;
            case FieldType::Array: typeOk = value->IsArray(); break;
            case FieldType::Object: typeOk = value->IsObject(); break;
            case FieldType::Any: typeOk = true; break;
        }
        if (!typeOk) {
            errors.push_back("'" + field.name + "' 필드는 " + FieldTypeToString(field.type) + " 타입이어야 합니다.");
            continue;
        }

        if (field.type == FieldType::Number) {
            double number = value->AsDouble();
            if (field.minValue && number < *field.minValue) {
                errors.push_back("'" + field.name + "' 필드 값은 " + std::to_string(*field.minValue) + " 이상이어야 합니다.");
            }
            if (field.maxValue && number > *field.maxValue) {
                errors.push_back("'" + field.name + "' 필드 값은 " + std::to_string(*field.maxValue) + " 이하여야 합니다.");
            }
        } else if (field.type == FieldType::String) {
            std::size_t length = value->AsString().size();
            if (field.minLength && length < *field.minLength) {
                errors.push_back("'" + field.name + "' 필드 길이는 " + std::to_string(*field.minLength) + " 이상이어야 합니다.");
            }
            if (field.maxLength && length > *field.maxLength) {
                errors.push_back("'" + field.name + "' 필드 길이는 " + std::to_string(*field.maxLength) + " 이하여야 합니다.");
            }
        }

        if (field.allowedValues) {
            bool allowed = std::any_of(field.allowedValues->begin(), field.allowedValues->end(),
                                        [&](const JsonValue& candidate) { return candidate == *value; });
            if (!allowed) {
                std::ostringstream allowedList;
                for (std::size_t i = 0; i < field.allowedValues->size(); ++i) {
                    if (i > 0) allowedList << ", ";
                    allowedList << DumpForMessage((*field.allowedValues)[i]);
                }
                errors.push_back("'" + field.name + "' 필드 값이 허용 목록에 없습니다. 허용값: [" + allowedList.str() + "]");
            }
        }
    }

    if (strict_ && record.IsObject()) {
        for (const auto& member : record.AsObject()) {
            if (member.first == ignoredField) continue;
            if (!HasField(member.first)) {
                errors.push_back("스키마에 정의되지 않은 필드입니다: '" + member.first + "'");
            }
        }
    }

    return errors;
}

JsonValue TableSchema::ToJson() const {
    JsonValue json = JsonValue::MakeObject();
    json["strict"] = JsonValue(strict_);
    JsonValue fieldArray = JsonValue::MakeArray();
    for (const auto& field : fields_) {
        fieldArray.AsArray().push_back(field.ToJson());
    }
    json["fields"] = fieldArray;
    return json;
}

TableSchema TableSchema::FromJson(const JsonValue& json) {
    if (!json.IsObject()) {
        throw SchemaException("Schema document must be a JSON object");
    }
    TableSchema schema;
    if (const JsonValue* strictValue = json.Find("strict")) {
        schema.SetStrict(strictValue->AsBool());
    }
    if (const JsonValue* fields = json.Find("fields")) {
        for (const auto& fieldJson : fields->AsArray()) {
            schema.AddField(FieldDefinition::FromJson(fieldJson));
        }
    }
    return schema;
}

} // namespace datamonitor
