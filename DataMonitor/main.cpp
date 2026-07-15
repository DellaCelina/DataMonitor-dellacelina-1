// DataMonitor - console demo for the generic DataMonitorCore JSON-backed
// data monitoring/CRUD engine. All semiconductor-domain knowledge lives in
// DemoSeeder; the engine itself (DataMonitorCore) and the CLI (MonitorCli)
// are fully generic over whatever tables happen to exist.
#include <iostream>

#include "datamonitor/JsonDatabase.h"
#include "datamonitor/MonitorService.h"

#include "DemoSeeder.h"
#include "MonitorCli.h"

int main() {
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
