#include "datamonitor/JsonTable.h"
#include "datamonitor/JsonFileStorage.h"

#include <filesystem>
#include <memory>

#include "TestFramework.h"

using datamonitor::JsonTable;
using datamonitor::JsonFileStorage;
using datamonitor::JsonValue;

namespace {

namespace fs = std::filesystem;

std::string TestDataPath(const std::string& fileName) {
    fs::create_directories("_test_data");
    return (fs::path("_test_data") / fileName).string();
}

// Removes a test-scratch file if present, so each test starts clean and
// leaves nothing behind for the next run.
struct ScopedTestFile {
    std::string path;
    explicit ScopedTestFile(std::string p) : path(std::move(p)) {
        std::error_code ec;
        fs::remove(path, ec);
        fs::remove(path + ".tmp", ec);
    }
    ~ScopedTestFile() {
        std::error_code ec;
        fs::remove(path, ec);
        fs::remove(path + ".tmp", ec);
    }
};

} // namespace

TEST(JsonTable_InsertAssignsIdWhenMissing) {
    ScopedTestFile file(TestDataPath("table_insert.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    JsonValue record = JsonValue::MakeObject();
    record["label"] = JsonValue(std::string("first"));
    JsonValue inserted = table.Insert(record);

    CHECK(inserted.Contains("id"));
    CHECK(!inserted["id"].AsString().empty());
    CHECK_EQ(table.Count(), static_cast<size_t>(1));
}

TEST(JsonTable_InsertKeepsExplicitId) {
    ScopedTestFile file(TestDataPath("table_insert_explicit_id.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    JsonValue record = JsonValue::MakeObject();
    record["id"] = JsonValue(std::string("custom-1"));
    record["label"] = JsonValue(std::string("first"));
    JsonValue inserted = table.Insert(record);

    CHECK_EQ(inserted["id"].AsString(), std::string("custom-1"));
}

TEST(JsonTable_GetFindAndFindByField) {
    ScopedTestFile file(TestDataPath("table_get_find.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    JsonValue a = JsonValue::MakeObject();
    a["kind"] = JsonValue(std::string("alpha"));
    JsonValue insertedA = table.Insert(a);

    JsonValue b = JsonValue::MakeObject();
    b["kind"] = JsonValue(std::string("beta"));
    table.Insert(b);

    auto found = table.Get(insertedA["id"].AsString());
    CHECK(found.has_value());
    CHECK_EQ(found->operator[]("kind").AsString(), std::string("alpha"));

    auto matches = table.Find([](const JsonValue& record) {
        return record["kind"].AsString() == "beta";
    });
    CHECK_EQ(matches.size(), static_cast<size_t>(1));

    auto byField = table.FindByField("kind", JsonValue(std::string("alpha")));
    CHECK_EQ(byField.size(), static_cast<size_t>(1));
    CHECK_EQ(byField[0]["id"].AsString(), insertedA["id"].AsString());
}

TEST(JsonTable_UpdateMergesWithoutClobbering) {
    ScopedTestFile file(TestDataPath("table_update.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    JsonValue record = JsonValue::MakeObject();
    record["kind"] = JsonValue(std::string("alpha"));
    record["count"] = JsonValue(1);
    JsonValue inserted = table.Insert(record);
    std::string id = inserted["id"].AsString();

    JsonValue patch = JsonValue::MakeObject();
    patch["count"] = JsonValue(2);
    bool updated = table.Update(id, patch);
    CHECK(updated);

    auto reloaded = table.Get(id);
    CHECK(reloaded.has_value());
    CHECK_EQ(reloaded->operator[]("count").AsDouble(), 2.0);
    CHECK_EQ(reloaded->operator[]("kind").AsString(), std::string("alpha"));
}

TEST(JsonTable_UpdateAndRemoveNonexistentReturnFalse) {
    ScopedTestFile file(TestDataPath("table_missing_ops.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    JsonValue patch = JsonValue::MakeObject();
    patch["x"] = JsonValue(1);
    CHECK(!table.Update("nope", patch));
    CHECK(!table.Remove("nope"));
    CHECK(!table.Get("nope").has_value());
}

TEST(JsonTable_RemoveDeletesRecord) {
    ScopedTestFile file(TestDataPath("table_remove.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    JsonValue inserted = table.Insert(JsonValue::MakeObject());
    std::string id = inserted["id"].AsString();
    CHECK_EQ(table.Count(), static_cast<size_t>(1));

    bool removed = table.Remove(id);
    CHECK(removed);
    CHECK_EQ(table.Count(), static_cast<size_t>(0));
    CHECK(!table.Get(id).has_value());
}

TEST(JsonTable_PersistenceSurvivesReopen) {
    ScopedTestFile file(TestDataPath("table_persist.json"));

    std::string id;
    {
        JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));
        JsonValue record = JsonValue::MakeObject();
        record["label"] = JsonValue(std::string("persisted"));
        id = table.Insert(record)["id"].AsString();
    }

    {
        JsonTable reopened("things", std::make_shared<JsonFileStorage>(file.path));
        CHECK_EQ(reopened.Count(), static_cast<size_t>(1));
        auto found = reopened.Get(id);
        CHECK(found.has_value());
        CHECK_EQ(found->operator[]("label").AsString(), std::string("persisted"));
    }
}

TEST(JsonTable_LoadingMissingFileYieldsEmptyTable) {
    std::string path = TestDataPath("table_does_not_exist.json");
    std::error_code ec;
    fs::remove(path, ec);

    JsonTable table("things", std::make_shared<JsonFileStorage>(path));
    CHECK_EQ(table.Count(), static_cast<size_t>(0));
    CHECK(table.All().empty());

    fs::remove(path, ec);
}
