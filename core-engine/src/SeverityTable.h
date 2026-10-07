#ifndef SEVERITY_TABLE_H
#define SEVERITY_TABLE_H

#include <string>
#include "HashTable.h"

// Maps a structured emergency type (selected by the caller/operator, NOT free text)
// to a fixed severity score. Higher score = more critical.
//
// This mirrors how real EMS dispatch protocols work: severity is assigned via a
// structured type/category, not inferred from parsing a free-text description.
//
// Backed by a HashTable<std::string, int> so lookup is O(1) average, and so the
// project consistently reuses the same HashTable class across every lookup need
// (ambulances, hospitals, and now severity types).
class SeverityTable {
private:
    HashTable<std::string, int> table;

    void loadDefaults();

public:
    SeverityTable();

    // Returns the severity score for a given emergency type.
    // If the type is unrecognized, returns a safe default (defaultSeverity) rather
    // than crashing - an unrecognized type should never silently get severity 0
    // and fall to the very bottom of the Priority Queue.
    int getSeverity(const std::string& emergencyType) const;

    // Allows adding/overriding an emergency type's severity at runtime, in case
    // the operator protocol needs to be extended or tuned later.
    void setSeverity(const std::string& emergencyType, int severity);

    static const int DEFAULT_SEVERITY = 5; // moderate, used only if type is unrecognized
};

#endif // SEVERITY_TABLE_H
