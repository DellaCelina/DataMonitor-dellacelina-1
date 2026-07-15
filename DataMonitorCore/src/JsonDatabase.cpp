#include "datamonitor/JsonDatabase.h"

#include <filesystem>
#include <set>

#include "datamonitor/JsonFileStorage.h"

namespace datamonitor {

namespace fs = std::filesystem;

JsonDatabase::JsonDatabase(std::string baseDirectory) : baseDirectory_(std::move(baseDirectory)) {
    std::error_code ec;
    fs::create_directories(baseDirectory_, ec);
}

std::string JsonDatabase::TablePath(const std::string& name) const {
    fs::path path(baseDirectory_);
    path /= (name + ".json");
    return path.string();
}

std::string JsonDatabase::SchemaPath(const std::string& name) const {
    fs::path path(baseDirectory_);
    path /= (name + ".schema.json");
    return path.string();
}

ITable& JsonDatabase::GetTable(const std::string& name) {
    auto it = tables_.find(name);
    if (it != tables_.end()) {
        return *it->second;
    }
    auto storage = std::make_shared<JsonFileStorage>(TablePath(name));
    auto schemaStorage = std::make_shared<JsonFileStorage>(SchemaPath(name));
    auto table = std::make_unique<JsonTable>(name, storage, "id", schemaStorage);
    ITable& ref = *table;
    tables_.emplace(name, std::move(table));
    return ref;
}

ITable& JsonDatabase::CreateTable(const std::string& name) {
    return GetTable(name);
}

std::vector<std::string> JsonDatabase::TableNames() const {
    std::set<std::string> names;
    for (const auto& kv : tables_) {
        names.insert(kv.first);
    }

    std::error_code ec;
    if (fs::exists(baseDirectory_, ec) && fs::is_directory(baseDirectory_, ec)) {
        for (const auto& entry : fs::directory_iterator(baseDirectory_, ec)) {
            if (!entry.is_regular_file()) continue;
            const fs::path& path = entry.path();
            const std::string filename = path.filename().string();
            // Skip "<name>.schema.json" sidecar files -- they are not
            // tables in their own right, just a table's schema metadata.
            const std::string schemaSuffix = ".schema.json";
            if (filename.size() >= schemaSuffix.size() &&
                filename.compare(filename.size() - schemaSuffix.size(), schemaSuffix.size(), schemaSuffix) == 0) {
                continue;
            }
            if (path.extension() == ".json") {
                names.insert(path.stem().string());
            }
        }
    }

    return std::vector<std::string>(names.begin(), names.end());
}

bool JsonDatabase::DropTable(const std::string& name) {
    bool existed = false;

    auto it = tables_.find(name);
    if (it != tables_.end()) {
        tables_.erase(it);
        existed = true;
    }

    std::error_code ec;

    std::string path = TablePath(name);
    if (fs::exists(path, ec)) {
        existed = true;
        fs::remove(path, ec);
    }

    std::string schemaPath = SchemaPath(name);
    if (fs::exists(schemaPath, ec)) {
        existed = true;
        fs::remove(schemaPath, ec);
    }

    return existed;
}

} // namespace datamonitor
