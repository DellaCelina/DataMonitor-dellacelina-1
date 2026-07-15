#include "DemoSeeder.h"

using datamonitor::JsonValue;

namespace demo {

void DemoSeeder::Seed(datamonitor::IDatabase& database) {
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
