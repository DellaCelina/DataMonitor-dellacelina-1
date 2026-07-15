#include "MonitorCli.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <vector>

#include "datamonitor/Schema.h"

using datamonitor::FieldDefinition;
using datamonitor::FieldType;
using datamonitor::FieldTypeFromString;
using datamonitor::FieldTypeToString;
using datamonitor::JsonValue;
using datamonitor::SchemaException;
using datamonitor::SchemaValidationException;
using datamonitor::TableSchema;

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

std::optional<double> ParseOptionalDouble(const std::string& line) {
    if (line.empty()) return std::nullopt;
    try {
        return std::stod(line);
    } catch (...) {
        return std::nullopt;
    }
}

std::optional<std::size_t> ParseOptionalSize(const std::string& line) {
    if (line.empty()) return std::nullopt;
    try {
        return static_cast<std::size_t>(std::stoul(line));
    } catch (...) {
        return std::nullopt;
    }
}

std::string Trim(const std::string& text) {
    auto isNotSpace = [](unsigned char c) { return std::isspace(c) == 0; };
    auto begin = std::find_if(text.begin(), text.end(), isNotSpace);
    auto end = std::find_if(text.rbegin(), text.rend(), isNotSpace).base();
    if (begin >= end) return "";
    return std::string(begin, end);
}

// Infers a JSON type from a raw "key=value" value token:
//   - wrapped in double quotes ("...")  -> string, quotes stripped
//   - "true" / "false"                  -> boolean
//   - fully parses as a number          -> number
//   - anything else                     -> string, taken as-is (unquoted words
//                                          like `name=샘플A` still work as before)
JsonValue ParseFieldValue(const std::string& rawValue) {
    std::string value = Trim(rawValue);

    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        return JsonValue(value.substr(1, value.size() - 2));
    }
    if (value == "true") return JsonValue(true);
    if (value == "false") return JsonValue(false);
    if (!value.empty()) {
        try {
            std::size_t consumed = 0;
            double number = std::stod(value, &consumed);
            if (consumed == value.size()) {
                return JsonValue(number);
            }
        } catch (...) {
            // Not a number -- fall through and store it as a string.
        }
    }
    return JsonValue(value);
}

} // namespace

void MonitorCli::PrintMainMenu() const {
    std::cout << "\n===== 데이터 모니터링 Tool =====\n";
    std::cout << "1. 테이블 요약 보기\n";
    std::cout << "2. 테이블 레코드 조회\n";
    std::cout << "3. 필드 검색\n";
    std::cout << "4. 레코드 추가\n";
    std::cout << "5. 레코드 삭제\n";
    std::cout << "6. 테이블 추가\n";
    std::cout << "7. 테이블 삭제\n";
    std::cout << "8. 스키마 관리\n";
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

    std::cout << "검색할 값 (숫자/true/false/문자열 자동 인식, 강제로 문자열로 찾으려면 큰따옴표로 감싸세요)> ";
    std::string value;
    if (!ReadLine(value)) return;

    auto results = service_.SearchRecords(table, field, ParseFieldValue(value));
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
    std::cout << "값은 숫자면 number, true/false면 boolean, 그 외에는 string으로 자동 인식됩니다.\n";
    std::cout << "숫자처럼 보여도 문자열로 넣고 싶다면 큰따옴표로 감싸세요 (예: id=\"123\").\n";
    std::cout << "예) id=\"123\",name=\"123\",num=10\n";
    std::cout << "입력> ";
    std::string line;
    if (!ReadLine(line)) return;

    JsonValue record = JsonValue::MakeObject();
    std::stringstream ss(line);
    std::string pair;
    while (std::getline(ss, pair, ',')) {
        auto eq = pair.find('=');
        if (eq == std::string::npos) continue;
        std::string key = Trim(pair.substr(0, eq));
        std::string value = pair.substr(eq + 1);
        if (key.empty()) continue;
        record[key] = ParseFieldValue(value);
    }

    try {
        JsonValue inserted = service_.AddRecord(table, record);
        std::cout << "추가되었습니다: " << inserted.Dump(false) << "\n";
    } catch (const SchemaValidationException& ex) {
        std::cout << "스키마 검증에 실패하여 추가되지 않았습니다:\n";
        for (const auto& error : ex.Errors()) {
            std::cout << "  - " << error << "\n";
        }
    }
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

void MonitorCli::CreateTableFlow() {
    std::cout << "생성할 테이블 이름> ";
    std::string name;
    if (!ReadLine(name) || name.empty()) return;

    service_.CreateTable(name);
    std::cout << "테이블 '" << name << "' 이(가) 준비되었습니다. (이미 있었다면 그대로 사용됩니다)\n";
}

void MonitorCli::DropTableFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "정말로 테이블 '" << table << "' 을(를) 삭제하시겠습니까? (y/N)> ";
    std::string confirm;
    if (!ReadLine(confirm)) return;
    if (confirm != "y" && confirm != "Y") {
        std::cout << "취소되었습니다.\n";
        return;
    }

    bool dropped = service_.DropTable(table);
    std::cout << (dropped ? "삭제되었습니다.\n" : "해당 테이블을 찾을 수 없습니다.\n");
}

void MonitorCli::PrintSchema(const std::string& table, const TableSchema& schema) const {
    if (schema.Empty()) {
        std::cout << "[" << table << "] 정의된 스키마가 없습니다 (자유 형식).\n";
        return;
    }
    std::cout << "[" << table << "] 스키마 (strict=" << (schema.IsStrict() ? "true" : "false") << ")\n";
    for (const auto& field : schema.Fields()) {
        std::cout << "  - " << field.name << " : " << FieldTypeToString(field.type)
                   << (field.required ? " (필수)" : " (선택)");
        if (field.minValue) std::cout << " min=" << *field.minValue;
        if (field.maxValue) std::cout << " max=" << *field.maxValue;
        if (field.minLength) std::cout << " minLength=" << *field.minLength;
        if (field.maxLength) std::cout << " maxLength=" << *field.maxLength;
        if (field.allowedValues) {
            std::cout << " allowedValues=[";
            for (std::size_t i = 0; i < field.allowedValues->size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << (*field.allowedValues)[i].Dump(false);
            }
            std::cout << "]";
        }
        std::cout << "\n";
    }
}

void MonitorCli::ShowSchemaFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    auto schema = service_.GetSchema(table);
    PrintSchema(table, schema.value_or(TableSchema()));
}

void MonitorCli::DefineFieldFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "필드 이름> ";
    std::string name;
    if (!ReadLine(name) || name.empty()) return;

    std::cout << "타입 (string/number/boolean/array/object/any, 기본 any)> ";
    std::string typeLine;
    if (!ReadLine(typeLine)) return;

    FieldDefinition field;
    field.name = name;
    try {
        field.type = typeLine.empty() ? FieldType::Any : FieldTypeFromString(typeLine);
    } catch (const SchemaException& ex) {
        std::cout << "잘못된 타입입니다: " << ex.what() << "\n";
        return;
    }

    std::cout << "필수 필드입니까? (y/N)> ";
    std::string requiredLine;
    if (!ReadLine(requiredLine)) return;
    field.required = (requiredLine == "y" || requiredLine == "Y");

    if (field.type == FieldType::Number) {
        std::cout << "최소값 (없으면 엔터)> ";
        std::string minLine;
        if (!ReadLine(minLine)) return;
        field.minValue = ParseOptionalDouble(minLine);

        std::cout << "최대값 (없으면 엔터)> ";
        std::string maxLine;
        if (!ReadLine(maxLine)) return;
        field.maxValue = ParseOptionalDouble(maxLine);
    } else if (field.type == FieldType::String) {
        std::cout << "최소 길이 (없으면 엔터)> ";
        std::string minLenLine;
        if (!ReadLine(minLenLine)) return;
        field.minLength = ParseOptionalSize(minLenLine);

        std::cout << "최대 길이 (없으면 엔터)> ";
        std::string maxLenLine;
        if (!ReadLine(maxLenLine)) return;
        field.maxLength = ParseOptionalSize(maxLenLine);
    }

    std::cout << "허용값 목록 (콤마로 구분, 없으면 엔터)> ";
    std::string allowedLine;
    if (!ReadLine(allowedLine)) return;
    if (!allowedLine.empty()) {
        std::vector<JsonValue> allowed;
        std::stringstream ss(allowedLine);
        std::string item;
        while (std::getline(ss, item, ',')) {
            if (!item.empty()) allowed.push_back(JsonValue(item));
        }
        if (!allowed.empty()) field.allowedValues = allowed;
    }

    bool added = service_.AddSchemaField(table, field);
    std::cout << (added ? "필드가 추가되었습니다.\n" : "이미 같은 이름의 필드가 존재합니다.\n");
}

void MonitorCli::RemoveFieldFlow() {
    std::string table = PromptTableName();
    if (table.empty()) return;

    std::cout << "삭제할 필드 이름> ";
    std::string field;
    if (!ReadLine(field)) return;

    bool removed = service_.RemoveSchemaField(table, field);
    std::cout << (removed ? "필드가 삭제되었습니다.\n" : "해당 필드(또는 스키마)를 찾을 수 없습니다.\n");
}

void MonitorCli::SchemaMenuFlow() {
    std::cout << "\n[스키마 관리]\n";
    std::cout << "1. 스키마 보기\n";
    std::cout << "2. 필드 추가\n";
    std::cout << "3. 필드 삭제\n";
    std::cout << "0. 뒤로\n";
    std::cout << "선택> ";

    std::string choice;
    if (!ReadLine(choice)) return;

    if (choice == "1") {
        ShowSchemaFlow();
    } else if (choice == "2") {
        DefineFieldFlow();
    } else if (choice == "3") {
        RemoveFieldFlow();
    }
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
        } else if (choiceLine == "6") {
            CreateTableFlow();
        } else if (choiceLine == "7") {
            DropTableFlow();
        } else if (choiceLine == "8") {
            SchemaMenuFlow();
        } else {
            std::cout << "알 수 없는 선택입니다.\n";
        }
    }
}

} // namespace demo
