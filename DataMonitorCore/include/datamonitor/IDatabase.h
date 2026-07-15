#pragma once
//
// IDatabase.h - abstract collection of named ITable instances.
//
#include <string>
#include <vector>

#include "datamonitor/ITable.h"

namespace datamonitor {

class IDatabase {
public:
    virtual ~IDatabase() = default;

    // Returns (lazily creating if needed) the table with the given name.
    virtual ITable& GetTable(const std::string& name) = 0;

    // Explicitly adds a table (the "CREATE TABLE" operation). Behaves the
    // same as GetTable when the table already exists -- this is purely an
    // intention-revealing alias for callers (e.g. a console menu's "add
    // table" option) that want to distinguish "I'm creating a table" from
    // "give me whatever table this is, creating it if needed".
    virtual ITable& CreateTable(const std::string& name) = 0;

    // Names of all tables known to this database (already opened this
    // session, plus any discovered on disk).
    virtual std::vector<std::string> TableNames() const = 0;

    // Removes a table's underlying storage entirely. Returns false if the
    // table did not exist.
    virtual bool DropTable(const std::string& name) = 0;
};

} // namespace datamonitor
