#include "datamonitor/JsonDatabase.h"

#include <algorithm>
#include <filesystem>

#include "TestFramework.h"

using datamonitor::JsonDatabase;
using datamonitor::JsonValue;

namespace {

namespace fs = std::filesystem;

// Each test gets its own scratch directory under _test_data, removed
// entirely before and after the test runs.
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

TEST(JsonDatabase_MultipleIndependentTables) {
    ScopedTestDir dir("_test_data/db_multi");
    JsonDatabase db(dir.path);

    auto& things = db.GetTable("things");
    things.Insert(JsonValue::MakeObject());

    auto& widgets = db.GetTable("widgets");
    widgets.Insert(JsonValue::MakeObject());
    widgets.Insert(JsonValue::MakeObject());

    CHECK_EQ(db.GetTable("things").Count(), static_cast<size_t>(1));
    CHECK_EQ(db.GetTable("widgets").Count(), static_cast<size_t>(2));
}

TEST(JsonDatabase_TableNamesReflectsReality) {
    ScopedTestDir dir("_test_data/db_names");
    JsonDatabase db(dir.path);

    db.GetTable("alpha").Insert(JsonValue::MakeObject());
    db.GetTable("beta").Insert(JsonValue::MakeObject());

    auto names = db.TableNames();
    std::sort(names.begin(), names.end());
    CHECK_EQ(names.size(), static_cast<size_t>(2));
    CHECK(std::find(names.begin(), names.end(), "alpha") != names.end());
    CHECK(std::find(names.begin(), names.end(), "beta") != names.end());
}

TEST(JsonDatabase_TableNamesDiscoversFilesFromDisk) {
    ScopedTestDir dir("_test_data/db_discover");
    {
        JsonDatabase db(dir.path);
        db.GetTable("gamma").Insert(JsonValue::MakeObject());
    }

    // Fresh database instance over the same directory, table never opened
    // in this instance -- should still be discovered via the *.json file.
    JsonDatabase db2(dir.path);
    auto names = db2.TableNames();
    CHECK(std::find(names.begin(), names.end(), "gamma") != names.end());
}

TEST(JsonDatabase_DropTableRemovesFile) {
    ScopedTestDir dir("_test_data/db_drop");
    JsonDatabase db(dir.path);
    db.GetTable("delta").Insert(JsonValue::MakeObject());

    std::string filePath = (fs::path(dir.path) / "delta.json").string();
    CHECK(fs::exists(filePath));

    bool dropped = db.DropTable("delta");
    CHECK(dropped);
    CHECK(!fs::exists(filePath));

    bool droppedAgain = db.DropTable("delta");
    CHECK(!droppedAgain);
}
