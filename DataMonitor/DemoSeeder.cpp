#include "DemoSeeder.h"

#include "datamonitor/Schema.h"

using datamonitor::FieldDefinition;
using datamonitor::FieldType;
using datamonitor::JsonValue;
using datamonitor::TableSchema;

namespace demo {

void DemoSeeder::DefineSampleSchema(datamonitor::IDatabase& database) {
    TableSchema schema;

    FieldDefinition nameField;
    nameField.name = "name";
    nameField.type = FieldType::String;
    nameField.required = true;
    nameField.minLength = 1;
    schema.AddField(nameField);

    FieldDefinition avgProductionMinutesField;
    avgProductionMinutesField.name = "avgProductionMinutes";
    avgProductionMinutesField.type = FieldType::Number;
    avgProductionMinutesField.required = true;
    avgProductionMinutesField.minValue = 0.0;
    schema.AddField(avgProductionMinutesField);

    FieldDefinition yieldField;
    yieldField.name = "yield";
    yieldField.type = FieldType::Number;
    yieldField.required = true;
    yieldField.minValue = 0.0;
    yieldField.maxValue = 1.0;
    schema.AddField(yieldField);

    FieldDefinition stockField;
    stockField.name = "stock";
    stockField.type = FieldType::Number;
    stockField.required = true;
    stockField.minValue = 0.0;
    schema.AddField(stockField);

    database.GetTable("samples").DefineSchema(schema);
}

void DemoSeeder::DefineOrderSchema(datamonitor::IDatabase& database) {
    TableSchema schema;

    FieldDefinition sampleIdField;
    sampleIdField.name = "sampleId";
    sampleIdField.type = FieldType::String;
    sampleIdField.required = true;
    sampleIdField.minLength = 1;
    schema.AddField(sampleIdField);

    FieldDefinition customerField;
    customerField.name = "customer";
    customerField.type = FieldType::String;
    customerField.required = true;
    customerField.minLength = 1;
    schema.AddField(customerField);

    FieldDefinition quantityField;
    quantityField.name = "quantity";
    quantityField.type = FieldType::Number;
    quantityField.required = true;
    quantityField.minValue = 1.0;
    schema.AddField(quantityField);

    FieldDefinition statusField;
    statusField.name = "status";
    statusField.type = FieldType::String;
    statusField.required = true;
    statusField.allowedValues = std::vector<JsonValue>{
        JsonValue(std::string("pending")),
        JsonValue(std::string("in_progress")),
        JsonValue(std::string("completed")),
        JsonValue(std::string("shipped")),
    };
    schema.AddField(statusField);

    database.GetTable("orders").DefineSchema(schema);
}

void DemoSeeder::Seed(datamonitor::IDatabase& database) {
    DefineSampleSchema(database);
    DefineOrderSchema(database);

    auto& samples = database.GetTable("samples");

    struct SampleSeed {
        const char* id;
        const char* name;
        double avgProductionMinutes;
        double yield;
        int stock;
    };
    static const SampleSeed sampleSeeds[] = {
        {"SMP-001", "8인치 웨이퍼 A타입", 42.5, 0.973, 120},
        {"SMP-002", "12인치 웨이퍼 B타입", 65.0, 0.951, 80},
        {"SMP-003", "테스트 다이 세트 C", 18.0, 0.990, 300},
        {"SMP-004", "패키징 샘플 D", 30.0, 0.965, 50},
    };
    for (const auto& s : sampleSeeds) {
        JsonValue record = JsonValue::MakeObject();
        record["id"] = JsonValue(std::string(s.id));
        record["name"] = JsonValue(std::string(s.name));
        record["avgProductionMinutes"] = JsonValue(s.avgProductionMinutes);
        record["yield"] = JsonValue(s.yield);
        record["stock"] = JsonValue(s.stock);
        samples.Insert(record);
    }

    auto& orders = database.GetTable("orders");
    struct OrderSeed {
        const char* id;
        const char* sampleId;
        const char* customer;
        int quantity;
        const char* status;
    };
    static const OrderSeed orderSeeds[] = {
        {"ORD-1001", "SMP-001", "㈜한빛전자", 40, "in_progress"},
        {"ORD-1002", "SMP-002", "대성반도체", 15, "pending"},
        {"ORD-1003", "SMP-001", "㈜한빛전자", 10, "completed"},
        {"ORD-1004", "SMP-003", "미래테크", 100, "pending"},
        {"ORD-1005", "SMP-004", "대성반도체", 25, "shipped"},
    };
    for (const auto& o : orderSeeds) {
        JsonValue record = JsonValue::MakeObject();
        record["id"] = JsonValue(std::string(o.id));
        record["sampleId"] = JsonValue(std::string(o.sampleId));
        record["customer"] = JsonValue(std::string(o.customer));
        record["quantity"] = JsonValue(o.quantity);
        record["status"] = JsonValue(std::string(o.status));
        orders.Insert(record);
    }
}

} // namespace demo
