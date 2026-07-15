#pragma once
//
// JsonFileStorage.h - IStorage backed by a single JSON file on disk.
//
#include <string>

#include "datamonitor/IStorage.h"

namespace datamonitor {

// Persists a JSON document to a single file. Writes are atomic: the new
// content is written to a temporary file in the same directory and then
// renamed over the destination, so a crash mid-write cannot corrupt the
// previously-saved data.
class JsonFileStorage : public IStorage {
public:
    explicit JsonFileStorage(std::string path);

    // Returns JsonValue::MakeArray() if the file does not exist yet.
    JsonValue Load() override;

    void Save(const JsonValue& value) override;

    const std::string& Path() const { return path_; }

private:
    std::string path_;
};

} // namespace datamonitor
