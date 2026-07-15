#pragma once
//
// DemoSeeder.h - domain-specific dummy data generator for the demo CLI.
//
// This is the ONLY place in the DataMonitor demo app that knows anything
// about the semiconductor-sample-management domain that inspired this
// project. DataMonitorCore itself has no idea "samples" or "orders" exist;
// it just stores whatever JSON objects a consumer hands it.
//
#include "datamonitor/IDatabase.h"

namespace demo {

class DemoSeeder {
public:
    // Defines a TableSchema for "samples" and "orders" (types, required
    // fields, numeric ranges, and an allowed-value list for order status),
    // then populates both tables with a handful of representative rows.
    // Intended to be called once, when the database is empty (see main.cpp).
    //
    // This demonstrates DataMonitorCore's schema feature end-to-end; the
    // schema definitions themselves are demo-domain knowledge and live only
    // here, never in DataMonitorCore.
    static void Seed(datamonitor::IDatabase& database);

private:
    static void DefineSampleSchema(datamonitor::IDatabase& database);
    static void DefineOrderSchema(datamonitor::IDatabase& database);
};

} // namespace demo
