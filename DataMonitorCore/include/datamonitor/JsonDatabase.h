#pragma once
//
// JsonDatabase.h - IDatabase where each table is "<baseDir>/<name>.json".
//
#include <map>
#include <memory>
#include <string>

#include "datamonitor/IDatabase.h"
#include "datamonitor/JsonTable.h"

namespace datamonitor {

class JsonDatabase : public IDatabase {
public:
    explicit JsonDatabase(std::string baseDirectory);

    ITable& GetTable(const std::string& name) override;

    std::vector<std::string> TableNames() const override;

    bool DropTable(const std::string& name) override;

    const std::string& BaseDirectory() const { return baseDirectory_; }

private:
    std::string TablePath(const std::string& name) const;

    std::string baseDirectory_;
    std::map<std::string, std::unique_ptr<JsonTable>> tables_;
};

} // namespace datamonitor
