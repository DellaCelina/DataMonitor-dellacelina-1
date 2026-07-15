#include "datamonitor/MonitorService.h"

#include <algorithm>

namespace datamonitor {

std::vector<std::pair<std::string, std::size_t>> MonitorService::Summary() const {
    std::vector<std::pair<std::string, std::size_t>> result;
    std::vector<std::string> names = database_.TableNames();
    std::sort(names.begin(), names.end());
    for (const auto& name : names) {
        ITable& table = database_.GetTable(name);
        result.emplace_back(name, table.Count());
    }
    return result;
}

std::vector<JsonValue> MonitorService::ListRecords(const std::string& table, std::size_t offset, std::size_t limit) const {
    ITable& t = database_.GetTable(table);
    std::vector<JsonValue> all = t.All();
    if (offset >= all.size()) return {};
    std::size_t end = std::min(all.size(), offset + limit);
    return std::vector<JsonValue>(all.begin() + static_cast<std::ptrdiff_t>(offset),
                                   all.begin() + static_cast<std::ptrdiff_t>(end));
}

std::vector<JsonValue> MonitorService::SearchRecords(const std::string& table, const std::string& field, const JsonValue& value) const {
    ITable& t = database_.GetTable(table);
    return t.FindByField(field, value);
}

JsonValue MonitorService::AddRecord(const std::string& table, JsonValue record) {
    ITable& t = database_.GetTable(table);
    return t.Insert(std::move(record));
}

bool MonitorService::DeleteRecord(const std::string& table, const std::string& id) {
    ITable& t = database_.GetTable(table);
    return t.Remove(id);
}

} // namespace datamonitor
