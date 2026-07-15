#pragma once
//
// MonitorCli.h - generic, table-agnostic console menu UI.
//
// Knows nothing about "samples" or "orders" specifically -- it discovers
// whatever tables exist via IDatabase::TableNames() and offers generic
// list/search/add/delete operations on top of MonitorService.
//
#include "datamonitor/MonitorService.h"
#include "datamonitor/Schema.h"

namespace demo {

class MonitorCli {
public:
    explicit MonitorCli(datamonitor::MonitorService& service) : service_(service) {}

    // Runs the interactive menu loop until the user chooses to quit or
    // standard input is exhausted (e.g. when piped non-interactively).
    void Run();

private:
    void PrintMainMenu() const;
    void ShowSummary() const;
    void ListRecordsFlow();
    void SearchFlow();
    void AddRecordFlow();
    void DeleteRecordFlow();
    void CreateTableFlow();
    void DropTableFlow();
    void SchemaMenuFlow();
    void DefineFieldFlow();
    void RemoveFieldFlow();
    void ShowSchemaFlow();

    std::string PromptTableName();
    void PrintSchema(const std::string& table, const datamonitor::TableSchema& schema) const;

    datamonitor::MonitorService& service_;
};

} // namespace demo
