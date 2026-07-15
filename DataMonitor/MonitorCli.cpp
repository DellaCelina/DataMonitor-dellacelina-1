#include "MonitorCli.h"

#include <iostream>
#include <limits>
#include <sstream>

using datamonitor::JsonValue;

namespace demo {

namespace {

// Reads one full line from stdin. Returns false at EOF (e.g. piped input
// ran out), so the caller can exit cleanly instead of looping forever.
bool ReadLine(std::string& out) {
    if (!std::getline(std::cin, out)) {
        return false;
    }
    return true;
}

} // namespace

void MonitorCli::PrintMainMenu() const {
    std::cout << "\n===== 데이터 모니터링 Tool =====\n";
    std::cout << "1. 테이블 요약 보기\n";
    std::cout << "2. 테이블 레코드 조회\n";
    std::cout << "3. 필드 검색\n";
    std::cout << "4. 레코드 추가\n";
    std::cout << "5. 레코드 삭제\n";
    std::cout << "0. 종료\n";
    std::cout << "선택> ";
}

void MonitorCli::ShowSummary() const {
    auto summary = service_.Summary();
    if (summary.empty()) {
        std::cout << "등록된 테이블이 없습니다.\n";
        return;
    }
    std::cout << "\n[테이블 요약]\n";
    for (const auto& entry : summary) {
        std::cout << "  - " << entry.first << " : " << entry.second << "건\n";
    }
}

std::string MonitorCli::PromptTableName() {
    auto summary = service_.Summary();
    if (summary.empty()) {
        std::cout << "등록된 테이블이 없습니다.\n";
        return "";
    }
    std::cout << "테이블 목록: ";
    for (std::size_t i = 0; i < summary.size(); ++i) {
        std::cout << summary[i].first;
        if (i + 1 < summary.size()) std::cout << ", ";
    }
    std::cout << "\n테이블 이름 입력> ";
    std::string name;
    if (!ReadLine(name)) return "";
    return name;
}

void MonitorCli::ListRecordsFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "시작 위치(offset, 기본 0)> ";
    std::string offsetLine;
    if (!ReadLine(offsetLine)) return;
    std::size_t offset = 0;
    if (!offsetLine.empty()) {
        try { offset = static_cast<std::size_t>(std::stoul(offsetLine)); } catch (...) { offset = 0; }
    }

    std::cout << "조회 개수(limit, 기본 10)> ";
    std::string limitLine;
    if (!ReadLine(limitLine)) return;
    std::size_t limit = 10;
    if (!limitLine.empty()) {
        try { limit = static_cast<std::size_t>(std::stoul(limitLine)); } catch (...) { limit = 10; }
    }

    auto records = service_.ListRecords(table, offset, limit);
    if (records.empty()) {
        std::cout << "조회된 레코드가 없습니다.\n";
        return;
    }
    std::cout << "\n[" << table << "] " << records.size() << "건\n";
    for (const auto& record : records) {
        std::cout << record.Dump(false) << "\n";
    }
}

void MonitorCli::SearchFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "검색할 필드명> ";
    std::string field;
    if (!ReadLine(field)) return;

    std::cout << "검색할 값> ";
    std::string value;
    if (!ReadLine(value)) return;

    auto results = service_.SearchRecords(table, field, JsonValue(value));
    if (results.empty()) {
        std::cout << "검색 결과가 없습니다.\n";
        return;
    }
    std::cout << "\n[검색 결과] " << results.size() << "건\n";
    for (const auto& record : results) {
        std::cout << record.Dump(false) << "\n";
    }
}

void MonitorCli::AddRecordFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "필드를 key=value 형태로 콤마(,)로 구분해 입력하세요.\n";
    std::cout << "예) name=샘플A,stock=10\n";
    std::cout << "입력> ";
    std::string line;
    if (!ReadLine(line)) return;

    JsonValue record = JsonValue::MakeObject();
    std::stringstream ss(line);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        std::string key = pair.substr(0, eq);
        std::string value = pair.substr(eq + 1);
        if (key.empty()) continue;
        record[key] = JsonValue(value);
    }

    JsonValue inserted = service_.AddRecord(table, record);
    std::cout << "추가되었습니다: " << inserted.Dump(false) << "\n";
}

void MonitorCli::DeleteRecordFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "삭제할 레코드의 id> ";
    std::string id;
    if (!ReadLine(id)) return;

    bool deleted = service_.DeleteRecord(table, id);
    std::cout << (deleted ? "삭제되었습니다.\n" : "해당 id를 찾을 수 없습니다.\n");
}

void MonitorCli::Run() {
    while (true) {
        PrintMainMenu();
        std::string choiceLine;
        if (!ReadLine(choiceLine)) {
            std::cout << "\n입력이 종료되어 프로그램을 종료합니다.\n";
            break;
        }
        if (choiceLine.empty()) continue;

        if (choiceLine == "0") {
            std::cout << "프로그램을 종료합니다.\n";
            break;
        } else if (choiceLine == "1") {
            ShowSummary();
        } else if (choiceLine == "2") {
            ListRecordsFlow();
        } else if (choiceLine == "3") {
            SearchFlow();
        } else if (choiceLine == "4") {
            AddRecordFlow();
        } else if (choiceLine == "5") {
            DeleteRecordFlow();
        } else {
            std::cout << "알 수 없는 선택입니다.\n";
        }
    }
}

} // namespace demo
