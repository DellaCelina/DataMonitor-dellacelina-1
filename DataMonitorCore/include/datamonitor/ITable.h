#pragma once
//
// ITable.h - generic CRUD interface over a collection of JSON records.
//
// Deliberately domain-agnostic: nothing here knows about samples, orders,
// or any other business concept. Any project can store its records as
// JSON objects and get insert/find/update/delete/list for free.
//
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "datamonitor/Json.h"
#include "datamonitor/Schema.h"

namespace datamonitor {

class ITable {
public:
    virtual ~ITable() = default;

    virtual const std::string& Name() const = 0;

    virtual std::vector<JsonValue> All() const = 0;

    virtual std::optional<JsonValue> Get(const std::string& id) const = 0;

    virtual std::vector<JsonValue> Find(const std::function<bool(const JsonValue&)>& predicate) const = 0;

    // Convenience search: records whose `field` equals `value`.
    virtual std::vector<JsonValue> FindByField(const std::string& field, const JsonValue& value) const = 0;

    // Inserts a new record, auto-generating an id field if absent/empty.
    // Returns the stored record (including the assigned id). If a schema
    // is defined (see DefineSchema), throws SchemaValidationException when
    // the record does not satisfy it -- nothing is persisted in that case.
    virtual JsonValue Insert(JsonValue record) = 0;

    // Merges `patch` fields into the existing record with the given id.
    // Returns false if no such record exists. If a schema is defined, the
    // *merged* record is validated before anything is persisted; throws
    // SchemaValidationException if it fails.
    virtual bool Update(const std::string& id, const JsonValue& patch) = 0;

    // Removes the record with the given id. Returns false if it did not exist.
    virtual bool Remove(const std::string& id) = 0;

    virtual std::size_t Count() const = 0;

    // ---- Schema management ("CREATE/ALTER TABLE"-style operations) ----
    //
    // A table's schema is entirely optional: with none defined, Insert/Update
    // behave exactly as before (any JSON object is accepted). Once a schema
    // is defined, every subsequent Insert/Update is validated against it.

    // Replaces the table's schema outright (persisted, if the underlying
    // implementation supports persistence).
    virtual void DefineSchema(const TableSchema& schema) = 0;

    // Returns the current schema, or std::nullopt if none has been defined.
    virtual std::optional<TableSchema> GetSchema() const = 0;

    // Removes the schema entirely; future inserts/updates go unvalidated.
    virtual void ClearSchema() = 0;

    // Adds a single field to the existing schema (creating an empty one
    // first if none exists yet) -- the "ALTER TABLE ADD COLUMN" operation.
    // Returns false (and makes no change) if a field with that name
    // already exists.
    virtual bool AddSchemaField(const FieldDefinition& field) = 0;

    // Removes a single field from the schema. Returns false if there is no
    // schema, or the schema has no field by that name.
    virtual bool RemoveSchemaField(const std::string& fieldName) = 0;
};

} // namespace datamonitor
