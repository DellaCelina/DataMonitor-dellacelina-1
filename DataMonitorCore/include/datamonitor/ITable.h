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
    // Returns the stored record (including the assigned id).
    virtual JsonValue Insert(JsonValue record) = 0;

    // Merges `patch` fields into the existing record with the given id.
    // Returns false if no such record exists.
    virtual bool Update(const std::string& id, const JsonValue& patch) = 0;

    // Removes the record with the given id. Returns false if it did not exist.
    virtual bool Remove(const std::string& id) = 0;

    virtual std::size_t Count() const = 0;
};

} // namespace datamonitor
