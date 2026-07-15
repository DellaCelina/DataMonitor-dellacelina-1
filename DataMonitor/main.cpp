// DataMonitor - console demo for the generic DataMonitorCore JSON-backed
// data monitoring/CRUD engine. All semiconductor-domain knowledge lives in
// DemoSeeder; the engine itself (DataMonitorCore) and the CLI (MonitorCli)
// are fully generic over whatever tables happen to exist.
#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "datamonitor/JsonDatabase.h"
#include "datamonitor/MonitorService.h"

#include "DemoSeeder.h"
#include "MonitorCli.h"

int main() {
#ifdef _WIN32
    // Source/execution charset is UTF-8 (see /utf-8 compiler flag), but the
    // Windows console defaults to the system ANSI/OEM codepage (e.g. CP949
    // on a Korean locale). Without switching the console itself to UTF-8,
    // every Korean byte sequence we print gets misinterpreted and shows up
    // as garbled text.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    datamonitor::JsonDatabase database("./data");

    // Seed demo data only the first time (samples table starts empty).
    if (database.GetTable("samples").Count() == 0) {
        demo::DemoSeeder::Seed(database);
        std::cout << "샘플 데이터를 생성했습니다. (./data 폴더에 저장됨)\n";
    }

    datamonitor::MonitorService service(database);
    demo::MonitorCli cli(service);
    cli.Run();

    return 0;
}
