#include "datamonitor/MonitorService.h"
#include "datamonitor/JsonDatabase.h"

#include <filesystem>

#include "TestFramework.h"

using datamonitor::JsonDatabase;
using datamonitor::MonitorService;
using datamonitor::JsonValue;

namespace {

namespace fs = std::filesystem;

struct ScopedTestDir {
    std::string path;
    explicit ScopedTestDir(std::string p) : path(std::move(p)) {
        std::error_code ec;
        fs::remove_all(path, ec);
        fs::create_directories(path, ec);
    }
    ~ScopedTestDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
};

} // namespace

TEST(MonitorService_SummaryMatchesTableCounts) {
    ScopedTestDir dir("_test_data/svc_summary");
    JsonDatabase db(dir.path);
    db.GetTable("things").Insert(JsonValue::MakeObject());
    db.GetTable("things").Insert(JsonValue::MakeObject());
    db.GetTable("widgets").Insert(JsonValue::MakeObject());

    MonitorService service(db);
    auto summary = service.Summary();

    bool foundThings = false, foundWidgets = false;
    for (const auto& entry : summary) {
        if (entry.first == "things") {
            CHECK_EQ(entry.second, static_cast<size_t>(2));
            foundThings = true;
        }
        if (entry.first == "widgets") {
            CHECK_EQ(entry.second, static_cast<size_t>(1));
            foundWidgets = true;
        }
    }
    CHECK(foundThings);
    CHECK(foundWidgets);
}

TEST(MonitorService_SearchReturnsExpectedSubset) {
    ScopedTestDir dir("_test_data/svc_search");
    JsonDatabase db(dir.path);
    MonitorService service(db);

    JsonValue a = JsonValue::MakeObject();
    a["status"] = JsonValue(std::string("open"));
    service.AddRecord("tickets", a);

    JsonValue b = JsonValue::MakeObject();
    b["status"] = JsonValue(std::string("closed"));
    service.AddRecord("tickets", b);

    JsonValue c = JsonValue::MakeObject();
    c["status"] = JsonValue(std::string("open"));
    service.AddRecord("tickets", c);

    auto openTickets = service.SearchRecords("tickets", "status", JsonValue(std::string("open")));
    CHECK_EQ(openTickets.size(), static_cast<size_t>(2));
}

TEST(MonitorService_AddAndDeletePropagateToTable) {
    ScopedTestDir dir("_test_data/svc_add_delete");
    JsonDatabase db(dir.path);
    MonitorService service(db);

    JsonValue record = JsonValue::MakeObject();
    record["label"] = JsonValue(std::string("hello"));
    JsonValue inserted = service.AddRecord("notes", record);
    std::string id = inserted["id"].AsString();

    CHECK_EQ(db.GetTable("notes").Count(), static_cast<size_t>(1));

    bool deleted = service.DeleteRecord("notes", id);
    CHECK(deleted);
    CHECK_EQ(db.GetTable("notes").Count(), static_cast<size_t>(0));
}

TEST(MonitorService_ListRecordsPaginates) {
    ScopedTestDir dir("_test_data/svc_list");
    JsonDatabase db(dir.path);
    MonitorService service(db);

    for (int i = 0; i < 5; ++i) {
        JsonValue record = JsonValue::MakeObject();
        record["n"] = JsonValue(i);
        service.AddRecord("items", record);
    }

    auto page = service.ListRecords("items", 2, 2);
    CHECK_EQ(page.size(), static_cast<size_t>(2));

    auto pastEnd = service.ListRecords("items", 10, 2);
    CHECK(pastEnd.empty());
}
