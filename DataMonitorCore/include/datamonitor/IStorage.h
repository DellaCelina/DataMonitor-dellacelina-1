#pragma once
//
// IStorage.h - abstract persistence backend for a single JSON document.
//
#include "datamonitor/Json.h"

namespace datamonitor {

class IStorage {
public:
    virtual ~IStorage() = default;

    // Loads and returns the stored JSON document. Implementations should
    // return an empty-but-valid JSON value (e.g. an empty array) if no data
    // exists yet, rather than throwing.
    virtual JsonValue Load() = 0;

    // Persists the given JSON document, replacing any previous contents.
    virtual void Save(const JsonValue& value) = 0;
};

} // namespace datamonitor
