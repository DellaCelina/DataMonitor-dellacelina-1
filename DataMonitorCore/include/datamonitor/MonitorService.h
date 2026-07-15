#pragma once
//
// MonitorService.h - thin, domain-agnostic facade over an IDatabase meant
// to back a console (or any other) monitoring UI.
//
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "datamonitor/IDatabase.h"
#include "datamonitor/Json.h"
#include "datamonitor/Schema.h"

namespace datamonitor {

class MonitorService {
public:
    explicit MonitorService(IDatabase& database) : database_(database) {}

    // Per-table record counts, one entry per table name (sorted by name).
    std::vector<std::pair<std::string, std::size_t>> Summary() const;

    // Returns up to `limit` records from `table` starting at `offset`.
    std::vector<JsonValue> ListRecords(const std::string& table, std::size_t offset, std::size_t limit) const;

    std::vector<JsonValue> SearchRecords(const std::string& table, const std::string& field, const JsonValue& value) const;

    // Throws SchemaValidationException if `table` has a schema and `record`
    // does not satisfy it.
    JsonValue AddRecord(const std::string& table, JsonValue record);

    bool DeleteRecord(const std::string& table, const std::string& id);

    // ---- Table management ----
    ITable& CreateTable(const std::string& name);
    bool DropTable(const std::string& name);

    // ---- Schema management ----
    void DefineSchema(const std::string& table, const TableSchema& schema);
    std::optional<TableSchema> GetSchema(const std::string& table) const;
    void ClearSchema(const std::string& table);
    bool AddSchemaField(const std::string& table, const FieldDefinition& field);
    bool RemoveSchemaField(const std::string& table, const std::string& fieldName);

    IDatabase& Database() { return database_; }

private:
    IDatabase& database_;
};

} // namespace datamonitor
