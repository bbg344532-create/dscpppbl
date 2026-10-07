#include "SeverityTable.h"

SeverityTable::SeverityTable() : table(101) {
    loadDefaults();
}

void SeverityTable::loadDefaults() {
    // Structured emergency-type -> severity lookup, loosely modeled on real
    // EMS triage categories. Higher = more critical.
    table.insert("Cardiac Arrest", 9);
    table.insert("Severe Trauma", 8);
    table.insert("Accident", 8);
    table.insert("Stroke", 8);
    table.insert("Breathing Difficulty", 7);
    table.insert("Severe Bleeding", 7);
    table.insert("Burns", 6);
    table.insert("Poisoning", 6);
    table.insert("High Fever", 4);
    table.insert("Fracture", 4);
    table.insert("Minor Injury", 2);
    table.insert("General Checkup", 1);
}

int SeverityTable::getSeverity(const std::string& emergencyType) const {
    int severity;
    if (table.find(emergencyType, severity)) {
        return severity;
    }
    return DEFAULT_SEVERITY; // unrecognized type - don't silently rank it lowest
}

void SeverityTable::setSeverity(const std::string& emergencyType, int severity) {
    table.insert(emergencyType, severity); // insert() already updates if key exists
}
