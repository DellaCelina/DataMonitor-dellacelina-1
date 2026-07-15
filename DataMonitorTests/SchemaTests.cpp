#include "datamonitor/Schema.h"
#include "datamonitor/JsonTable.h"
#include "datamonitor/JsonFileStorage.h"
#include "datamonitor/JsonDatabase.h"

#include <algorithm>
#include <filesystem>
#include <memory>

#include "TestFramework.h"

using datamonitor::FieldDefinition;
using datamonitor::FieldType;
using datamonitor::FieldTypeFromString;
using datamonitor::FieldTypeToString;
using datamonitor::JsonDatabase;
using datamonitor::JsonFileStorage;
using datamonitor::JsonTable;
using datamonitor::JsonValue;
using datamonitor::SchemaException;
using datamonitor::SchemaValidationException;
using datamonitor::TableSchema;

namespace {

namespace fs = std::filesystem;

std::string TestDataPath(const std::string& fileName) {
    fs::create_directories("_test_data");
    return (fs::path("_test_data") / fileName).string();
}

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

TableSchema MakeSampleSchema() {
    TableSchema schema;

    FieldDefinition name;
    name.name = "name";
    name.type = FieldType::String;
    name.required = true;
    name.minLength = 2;
    name.maxLength = 10;
    schema.AddField(name);

    FieldDefinition yield;
    yield.name = "yield";
    yield.type = FieldType::Number;
    yield.required = true;
    yield.minValue = 0.0;
    yield.maxValue = 1.0;
    schema.AddField(yield);

    FieldDefinition status;
    status.name = "status";
    status.type = FieldType::String;
    status.required = false;
    status.allowedValues = std::vector<JsonValue>{JsonValue(std::string("active")), JsonValue(std::string("retired"))};
    schema.AddField(status);

    return schema;
}

} // namespace

TEST(FieldType_ToStringFromStringRoundTrip) {
    for (FieldType type : {FieldType::String, FieldType::Number, FieldType::Boolean, FieldType::Array,
                           FieldType::Object, FieldType::Any}) {
        CHECK(FieldTypeFromString(FieldTypeToString(type)) == type);
    }
    CHECK_THROWS(FieldTypeFromString("not-a-type"));
}

TEST(FieldDefinition_JsonRoundTrip) {
    FieldDefinition field;
    field.name = "quantity";
    field.type = FieldType::Number;
    field.required = true;
    field.minValue = 1.0;
    field.maxValue = 1000.0;

    JsonValue json = field.ToJson();
    FieldDefinition roundTripped = FieldDefinition::FromJson(json);

    CHECK_EQ(roundTripped.name, field.name);
    CHECK(roundTripped.type == field.type);
    CHECK_EQ(roundTripped.required, field.required);
    CHECK(roundTripped.minValue.has_value());
    CHECK_EQ(*roundTripped.minValue, 1.0);
    CHECK(roundTripped.maxValue.has_value());
    CHECK_EQ(*roundTripped.maxValue, 1000.0);
}

TEST(TableSchema_AddFieldRejectsDuplicateOrEmptyName) {
    TableSchema schema = MakeSampleSchema();
    FieldDefinition dup;
    dup.name = "name";
    CHECK_THROWS(schema.AddField(dup));

    FieldDefinition empty;
    empty.name = "";
    CHECK_THROWS(schema.AddField(empty));
}

TEST(TableSchema_RemoveAndHasField) {
    TableSchema schema = MakeSampleSchema();
    CHECK(schema.HasField("yield"));
    CHECK(schema.RemoveField("yield"));
    CHECK(!schema.HasField("yield"));
    CHECK(!schema.RemoveField("yield"));
}

TEST(TableSchema_ValidateRequiredMissingField) {
    TableSchema schema = MakeSampleSchema();
    JsonValue record = JsonValue::MakeObject();
    record["yield"] = JsonValue(0.5);
    // "name" is required and missing.
    auto errors = schema.Validate(record);
    CHECK(!errors.empty());
}

TEST(TableSchema_ValidateTypeMismatch) {
    TableSchema schema = MakeSampleSchema();
    JsonValue record = JsonValue::MakeObject();
    record["name"] = JsonValue(std::string("ok-name"));
    record["yield"] = JsonValue(std::string("not-a-number"));
    auto errors = schema.Validate(record);
    CHECK(!errors.empty());
}

TEST(TableSchema_ValidateNumberRange) {
    TableSchema schema = MakeSampleSchema();
    JsonValue record = JsonValue::MakeObject();
    record["name"] = JsonValue(std::string("ok-name"));
    record["yield"] = JsonValue(1.5); // > max 1.0
    auto errors = schema.Validate(record);
    CHECK(!errors.empty());
}

TEST(TableSchema_ValidateStringLength) {
    TableSchema schema = MakeSampleSchema();
    JsonValue record = JsonValue::MakeObject();
    record["name"] = JsonValue(std::string("a")); // shorter than minLength 2
    record["yield"] = JsonValue(0.5);
    auto errors = schema.Validate(record);
    CHECK(!errors.empty());
}

TEST(TableSchema_ValidateAllowedValues) {
    TableSchema schema = MakeSampleSchema();
    JsonValue record = JsonValue::MakeObject();
    record["name"] = JsonValue(std::string("ok-name"));
    record["yield"] = JsonValue(0.5);
    record["status"] = JsonValue(std::string("unknown-status"));
    auto errors = schema.Validate(record);
    CHECK(!errors.empty());

    record["status"] = JsonValue(std::string("active"));
    CHECK(schema.Validate(record).empty());
}

TEST(TableSchema_StrictModeRejectsUnknownFields) {
    TableSchema schema = MakeSampleSchema();
    schema.SetStrict(true);

    JsonValue record = JsonValue::MakeObject();
    record["name"] = JsonValue(std::string("ok-name"));
    record["yield"] = JsonValue(0.5);
    record["id"] = JsonValue(std::string("123")); // exempt via ignoredField
    record["extra"] = JsonValue(std::string("unexpected"));

    auto errors = schema.Validate(record, "id");
    CHECK(!errors.empty());

    record.AsObject().pop_back(); // drop "extra"
    CHECK(schema.Validate(record, "id").empty());
}

TEST(TableSchema_JsonRoundTrip) {
    TableSchema schema = MakeSampleSchema();
    schema.SetStrict(true);

    JsonValue json = schema.ToJson();
    TableSchema roundTripped = TableSchema::FromJson(json);

    CHECK_EQ(roundTripped.IsStrict(), true);
    CHECK_EQ(roundTripped.Fields().size(), schema.Fields().size());
    CHECK(roundTripped.HasField("name"));
    CHECK(roundTripped.HasField("yield"));
    CHECK(roundTripped.HasField("status"));
}

TEST(JsonTable_InsertValidatesAgainstSchema) {
    ScopedTestFile file(TestDataPath("schema_insert.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));
    table.DefineSchema(MakeSampleSchema());

    JsonValue valid = JsonValue::MakeObject();
    valid["name"] = JsonValue(std::string("ok-name"));
    valid["yield"] = JsonValue(0.5);
    CHECK_EQ(table.Insert(valid)["name"].AsString(), std::string("ok-name"));
    CHECK_EQ(table.Count(), static_cast<size_t>(1));

    JsonValue invalid = JsonValue::MakeObject();
    invalid["name"] = JsonValue(std::string("a")); // too short
    invalid["yield"] = JsonValue(0.5);
    CHECK_THROWS(table.Insert(invalid));
    // The failed insert must not have been persisted.
    CHECK_EQ(table.Count(), static_cast<size_t>(1));
}

TEST(JsonTable_UpdateValidatesMergedRecordAgainstSchema) {
    ScopedTestFile file(TestDataPath("schema_update.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));
    table.DefineSchema(MakeSampleSchema());

    JsonValue record = JsonValue::MakeObject();
    record["name"] = JsonValue(std::string("ok-name"));
    record["yield"] = JsonValue(0.5);
    std::string id = table.Insert(record)["id"].AsString();

    JsonValue badPatch = JsonValue::MakeObject();
    badPatch["yield"] = JsonValue(5.0); // exceeds max 1.0
    CHECK_THROWS(table.Update(id, badPatch));

    // Unaffected by the failed update.
    auto reloaded = table.Get(id);
    CHECK(reloaded.has_value());
    CHECK_EQ(reloaded->operator[]("yield").AsDouble(), 0.5);

    JsonValue goodPatch = JsonValue::MakeObject();
    goodPatch["yield"] = JsonValue(0.9);
    CHECK(table.Update(id, goodPatch));
    CHECK_EQ(table.Get(id)->operator[]("yield").AsDouble(), 0.9);
}

TEST(JsonTable_ClearSchemaRemovesValidation) {
    ScopedTestFile file(TestDataPath("schema_clear.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));
    table.DefineSchema(MakeSampleSchema());

    JsonValue invalid = JsonValue::MakeObject();
    invalid["name"] = JsonValue(std::string("a"));
    CHECK_THROWS(table.Insert(invalid));

    table.ClearSchema();
    CHECK(!table.GetSchema().has_value());
    CHECK_EQ(table.Insert(invalid)["name"].AsString(), std::string("a"));
}

TEST(JsonTable_AddAndRemoveSchemaFieldMutatesInPlace) {
    ScopedTestFile file(TestDataPath("schema_add_remove_field.json"));
    JsonTable table("things", std::make_shared<JsonFileStorage>(file.path));

    CHECK(!table.GetSchema().has_value());

    FieldDefinition field;
    field.name = "label";
    field.type = FieldType::String;
    field.required = true;
    CHECK(table.AddSchemaField(field));
    CHECK(table.GetSchema().has_value());
    CHECK(table.GetSchema()->HasField("label"));

    // Adding the same field name again should fail without disturbing the schema.
    CHECK(!table.AddSchemaField(field));

    CHECK(table.RemoveSchemaField("label"));
    CHECK(!table.GetSchema()->HasField("label"));
    CHECK(!table.RemoveSchemaField("label"));
}

TEST(JsonTable_SchemaPersistsAcrossReopen) {
    ScopedTestFile dataFile(TestDataPath("schema_persist_data.json"));
    ScopedTestFile schemaFile(TestDataPath("schema_persist_schema.json"));

    {
        JsonTable table("things", std::make_shared<JsonFileStorage>(dataFile.path), "id",
                         std::make_shared<JsonFileStorage>(schemaFile.path));
        table.DefineSchema(MakeSampleSchema());
    }

    {
        JsonTable reopened("things", std::make_shared<JsonFileStorage>(dataFile.path), "id",
                            std::make_shared<JsonFileStorage>(schemaFile.path));
        auto schema = reopened.GetSchema();
        CHECK(schema.has_value());
        CHECK(schema->HasField("name"));
        CHECK(schema->HasField("yield"));

        JsonValue invalid = JsonValue::MakeObject();
        invalid["name"] = JsonValue(std::string("a"));
        CHECK_THROWS(reopened.Insert(invalid));
    }
}

TEST(JsonDatabase_CreateTableIsIdempotentWithGetTable) {
    ScopedTestDir dir(TestDataPath("schema_db_create"));
    JsonDatabase database(dir.path);

    auto& created = database.CreateTable("widgets");
    created.Insert(JsonValue::MakeObject());

    auto& again = database.GetTable("widgets");
    CHECK_EQ(again.Count(), static_cast<size_t>(1));
}

TEST(JsonDatabase_SchemaSurvivesDatabaseReopenAndDropRemovesSchemaFile) {
    ScopedTestDir dir(TestDataPath("schema_db_reopen"));

    {
        JsonDatabase database(dir.path);
        database.GetTable("widgets").DefineSchema(MakeSampleSchema());
    }

    std::string schemaPath = (fs::path(dir.path) / "widgets.schema.json").string();
    CHECK(fs::exists(schemaPath));

    {
        JsonDatabase database(dir.path);
        auto schema = database.GetTable("widgets").GetSchema();
        CHECK(schema.has_value());
        CHECK(schema->HasField("name"));

        // The schema sidecar file must never show up as its own table.
        auto names = database.TableNames();
        CHECK(std::find(names.begin(), names.end(), std::string("widgets")) != names.end());
        CHECK(std::find(names.begin(), names.end(), std::string("widgets.schema")) == names.end());

        CHECK(database.DropTable("widgets"));
    }

    CHECK(!fs::exists(schemaPath));
}
