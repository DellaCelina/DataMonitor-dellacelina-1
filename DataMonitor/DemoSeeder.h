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
    // Populates the "samples" and "orders" tables with a handful of
    // representative rows. Intended to be called once, when the database
    // is empty (see main.cpp).
    static void Seed(datamonitor::IDatabase& database);
};

} // namespace demo
