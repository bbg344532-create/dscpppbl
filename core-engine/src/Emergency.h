#ifndef EMERGENCY_H
#define EMERGENCY_H

#include "Hospital.h" // for Specialization enum

class Emergency {
private:
    int id;
    int locationId;         // node id where emergency was reported
    int severity;           // lower = more critical
    Specialization requiredSpecialization;
    long timestamp;

public:
    Emergency(int id, int locationId, int severity,
              Specialization requiredSpec, long timestamp);

    int getId() const;
    int getLocationId() const;
    int getSeverity() const;
    Specialization getRequiredSpecialization() const;
    long getTimestamp() const;
};

#endif // EMERGENCY_H
