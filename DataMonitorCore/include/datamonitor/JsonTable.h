#pragma once
//
// JsonTable.h - ITable implementation backed by an IStorage of JSON records.
//
#include <memory>
#include <string>

#include "datamonitor/ITable.h"
#include "datamonitor/IStorage.h"

namespace datamonitor {

// Concrete, general-purpose table: keeps an in-memory cache of records
// (loaded once via an IStorage), and re-persists the whole record array
// after every mutating call. The id field name is configurable so a
// consumer can reuse a different primary-key convention if desired.
class JsonTable : public ITable {
public:
    JsonTable(std::string name, std::shared_ptr<IStorage> storage, std::string idField = "id");

    const std::string& Name() const override { return name_; }

    std::vector<JsonValue> All() const override;

    std::optional<JsonValue> Get(const std::string& id) const override;

    std::vector<JsonValue> Find(const std::function<bool(const JsonValue&)>& predicate) const override;

    std::vector<JsonValue> FindByField(const std::string& field, const JsonValue& value) const override;

    JsonValue Insert(JsonValue record) override;

    bool Update(const std::string& id, const JsonValue& patch) override;

    bool Remove(const std::string& id) override;

    std::size_t Count() const override;

private:
    void Load();
    void Persist() const;
    std::string GenerateId() const;
    int FindIndexById(const std::string& id) const;

    std::string name_;
    std::shared_ptr<IStorage> storage_;
    std::string idField_;
    JsonValue::Array records_;
    mutable unsigned long long nextNumericId_ = 1;
};

} // namespace datamonitor
