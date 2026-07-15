#pragma once
//
// Schema.h - optional per-table schema definition and validation.
//
// Deliberately generic: a TableSchema is just a named list of field
// constraints (type, required-ness, numeric range, string length, and an
// enum-like allowed-value list). Any project can attach a TableSchema to
// any ITable to get DB-style "CREATE TABLE" / "ALTER TABLE ADD COLUMN" /
// constraint-checked-insert behavior on top of the otherwise schemaless
// JSON record store. Nothing here knows about samples, orders, or any
// other business concept.
//
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "datamonitor/Json.h"

namespace datamonitor {

// The JSON type a field's value is expected to hold. `Any` opts a field out
// of type checking (useful when only presence/required-ness matters).
enum class FieldType {
    String,
    Number,
    Boolean,
    Array,
    Object,
    Any
};

std::string FieldTypeToString(FieldType type);
FieldType FieldTypeFromString(const std::string& text);

// Thrown for malformed schema *definitions* (duplicate field name, unknown
// type string, malformed schema JSON) -- as opposed to SchemaValidationException,
// which is thrown when a *record* fails to satisfy an otherwise-valid schema.
class SchemaException : public std::runtime_error {
public:
    explicit SchemaException(const std::string& message) : std::runtime_error(message) {}
};

// Thrown by JsonTable::Insert/Update when a record does not satisfy the
// table's schema. Carries every violation found, not just the first.
class SchemaValidationException : public std::runtime_error {
public:
    explicit SchemaValidationException(std::vector<std::string> errors);

    const std::vector<std::string>& Errors() const { return errors_; }

private:
    std::vector<std::string> errors_;
};

// One field's constraints within a TableSchema.
struct FieldDefinition {
    std::string name;
    FieldType type = FieldType::Any;
    bool required = false;

    // Only meaningful when type == Number.
    std::optional<double> minValue;
    std::optional<double> maxValue;

    // Only meaningful when type == String.
    std::optional<std::size_t> minLength;
    std::optional<std::size_t> maxLength;

    // Enum-like constraint: if set, the field's value must equal one of
    // these. Works for any type (commonly String, e.g. a status field).
    std::optional<std::vector<JsonValue>> allowedValues;

    JsonValue ToJson() const;
    static FieldDefinition FromJson(const JsonValue& json);
};

// A table's schema: an ordered set of field definitions plus a "strict"
// flag controlling whether records may carry fields the schema doesn't know
// about.
class TableSchema {
public:
    TableSchema() = default;

    // Adds a field definition. Throws SchemaException if the name is empty
    // or a field with that name already exists.
    void AddField(FieldDefinition field);

    // Removes the field with the given name. Returns false if it did not exist.
    bool RemoveField(const std::string& name);

    bool HasField(const std::string& name) const;
    std::optional<FieldDefinition> GetField(const std::string& name) const;
    const std::vector<FieldDefinition>& Fields() const { return fields_; }
    bool Empty() const { return fields_.empty(); }

    // When true, Validate() also rejects any record field not declared in
    // the schema (aside from `ignoredField`, typically the table's id field).
    bool IsStrict() const { return strict_; }
    void SetStrict(bool strict) { strict_ = strict; }

    // Checks `record` against every field definition (and, if strict,
    // against unknown fields). Returns a human-readable error per
    // violation; an empty vector means the record is valid. `ignoredField`
    // (typically the table's id field name) is always allowed even in
    // strict mode, since ids are a storage concern, not a schema concern.
    std::vector<std::string> Validate(const JsonValue& record, const std::string& ignoredField = "id") const;

    JsonValue ToJson() const;
    static TableSchema FromJson(const JsonValue& json);

private:
    std::vector<FieldDefinition> fields_;
    bool strict_ = false;
};

} // namespace datamonitor
