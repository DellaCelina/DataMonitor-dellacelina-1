#include "datamonitor/JsonTable.h"

#include <algorithm>
#include <cctype>

namespace datamonitor {

JsonTable::JsonTable(std::string name, std::shared_ptr<IStorage> storage, std::string idField,
                     std::shared_ptr<IStorage> schemaStorage)
    : name_(std::move(name)), storage_(std::move(storage)), idField_(std::move(idField)),
      schemaStorage_(std::move(schemaStorage)) {
    Load();
    LoadSchema();
}

void JsonTable::Load() {
    JsonValue document = storage_->Load();
    if (document.IsArray()) {
        records_ = document.AsArray();
    } else {
        // Tolerate an empty/absent/wrongly-shaped document by starting empty.
        records_ = JsonValue::Array{};
    }

    nextNumericId_ = 1;
    for (const auto& record : records_) {
        const JsonValue* idValue = record.Find(idField_);
        if (!idValue || !idValue->IsString()) continue;
        const std::string& idString = idValue->AsString();
        if (!idString.empty() &&
            std::all_of(idString.begin(), idString.end(), [](unsigned char c) { return std::isdigit(c) != 0; })) {
            try {
                unsigned long long numeric = std::stoull(idString);
                if (numeric >= nextNumericId_) nextNumericId_ = numeric + 1;
            } catch (const std::exception&) {
                // Not representable as an integer; ignore for id-sequence purposes.
            }
        }
    }
}

void JsonTable::Persist() const {
    storage_->Save(JsonValue(records_));
}

void JsonTable::LoadSchema() {
    if (!schemaStorage_) return;
    JsonValue document = schemaStorage_->Load();
    // JsonFileStorage returns an empty Array for a nonexistent file, which
    // is how "no schema has been defined yet" is represented on disk.
    if (!document.IsObject()) {
        schema_ = std::nullopt;
        return;
    }
    schema_ = TableSchema::FromJson(document);
}

void JsonTable::PersistSchema() const {
    if (!schemaStorage_) return;
    if (schema_) {
        schemaStorage_->Save(schema_->ToJson());
    } else {
        schemaStorage_->Save(JsonValue::MakeArray());
    }
}

std::string JsonTable::GenerateId() const {
    while (true) {
        std::string candidate = std::to_string(nextNumericId_++);
        bool collides = std::any_of(records_.begin(), records_.end(), [&](const JsonValue& record) {
            const JsonValue* idValue = record.Find(idField_);
            return idValue && idValue->IsString() && idValue->AsString() == candidate;
        });
        if (!collides) return candidate;
    }
}

int JsonTable::FindIndexById(const std::string& id) const {
    for (std::size_t i = 0; i < records_.size(); ++i) {
        const JsonValue* idValue = records_[i].Find(idField_);
        if (idValue && idValue->IsString() && idValue->AsString() == id) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::vector<JsonValue> JsonTable::All() const {
    return std::vector<JsonValue>(records_.begin(), records_.end());
}

std::optional<JsonValue> JsonTable::Get(const std::string& id) const {
    int index = FindIndexById(id);
    if (index < 0) return std::nullopt;
    return records_[static_cast<std::size_t>(index)];
}

std::vector<JsonValue> JsonTable::Find(const std::function<bool(const JsonValue&)>& predicate) const {
    std::vector<JsonValue> result;
    for (const auto& record : records_) {
        if (predicate(record)) result.push_back(record);
    }
    return result;
}

std::vector<JsonValue> JsonTable::FindByField(const std::string& field, const JsonValue& value) const {
    return Find([&](const JsonValue& record) {
        const JsonValue* fieldValue = record.Find(field);
        return fieldValue != nullptr && *fieldValue == value;
    });
}

JsonValue JsonTable::Insert(JsonValue record) {
    if (!record.IsObject()) {
        throw JsonException("JsonTable::Insert requires a JSON object record");
    }
    if (schema_) {
        std::vector<std::string> errors = schema_->Validate(record, idField_);
        if (!errors.empty()) {
            throw SchemaValidationException(std::move(errors));
        }
    }
    const JsonValue* existingId = record.Find(idField_);
    bool needsId = existingId == nullptr || !existingId->IsString() || existingId->AsString().empty();
    if (needsId) {
        record[idField_] = JsonValue(GenerateId());
    }
    records_.push_back(record);
    Persist();
    return record;
}

bool JsonTable::Update(const std::string& id, const JsonValue& patch) {
    int index = FindIndexById(id);
    if (index < 0) return false;
    JsonValue& target = records_[static_cast<std::size_t>(index)];

    if (schema_) {
        // Validate the record as it would look *after* the patch, so
        // required/typed fields already present can't be silently broken
        // by a partial update.
        JsonValue merged = target;
        merged.MergeFrom(patch);
        merged[idField_] = JsonValue(id);
        std::vector<std::string> errors = schema_->Validate(merged, idField_);
        if (!errors.empty()) {
            throw SchemaValidationException(std::move(errors));
        }
    }

    target.MergeFrom(patch);
    // Guard against a patch overwriting the id with something else.
    target[idField_] = JsonValue(id);
    Persist();
    return true;
}

bool JsonTable::Remove(const std::string& id) {
    int index = FindIndexById(id);
    if (index < 0) return false;
    records_.erase(records_.begin() + index);
    Persist();
    return true;
}

std::size_t JsonTable::Count() const {
    return records_.size();
}

void JsonTable::DefineSchema(const TableSchema& schema) {
    schema_ = schema;
    PersistSchema();
}

void JsonTable::ClearSchema() {
    schema_ = std::nullopt;
    PersistSchema();
}

bool JsonTable::AddSchemaField(const FieldDefinition& field) {
    if (!schema_) {
        schema_ = TableSchema();
    }
    if (schema_->HasField(field.name)) {
        return false;
    }
    schema_->AddField(field);
    PersistSchema();
    return true;
}

bool JsonTable::RemoveSchemaField(const std::string& fieldName) {
    if (!schema_) return false;
    bool removed = schema_->RemoveField(fieldName);
    if (removed) PersistSchema();
    return removed;
}

} // namespace datamonitor
