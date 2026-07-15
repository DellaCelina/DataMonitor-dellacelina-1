#include "datamonitor/JsonFileStorage.h"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace datamonitor {

namespace fs = std::filesystem;

JsonFileStorage::JsonFileStorage(std::string path) : path_(std::move(path)) {}

JsonValue JsonFileStorage::Load() {
    std::ifstream in(path_, std::ios::binary);
    if (!in.is_open()) {
        return JsonValue::MakeArray();
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string content = buffer.str();
    if (content.empty()) {
        return JsonValue::MakeArray();
    }
    return JsonValue::Parse(content);
}

void JsonFileStorage::Save(const JsonValue& value) {
    fs::path target(path_);
    if (target.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(target.parent_path(), ec);
    }

    fs::path tempPath = target;
    tempPath += ".tmp";

    {
        std::ofstream out(tempPath, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            throw std::runtime_error("Failed to open temp file for writing: " + tempPath.string());
        }
        std::string dumped = value.Dump(true);
        out.write(dumped.data(), static_cast<std::streamsize>(dumped.size()));
        out.flush();
        if (!out.good()) {
            throw std::runtime_error("Failed to write temp file: " + tempPath.string());
        }
    }

    std::error_code ec;
    fs::rename(tempPath, target, ec);
    if (ec) {
        // Cross-volume or platform quirk fallback: remove destination then rename.
        fs::remove(target, ec);
        fs::rename(tempPath, target, ec);
        if (ec) {
            throw std::runtime_error("Failed to atomically replace file: " + target.string());
        }
    }
}

} // namespace datamonitor
