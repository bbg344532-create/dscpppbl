#include "Emergency.h"

Emergency::Emergency(int id, int locationId, int severity,
                      Specialization requiredSpec, long timestamp)
    : id(id), locationId(locationId), severity(severity),
      requiredSpecialization(requiredSpec), timestamp(timestamp) {}

int Emergency::getId() const { return id; }

int Emergency::getLocationId() const { return locationId; }

int Emergency::getSeverity() const { return severity; }

Specialization Emergency::getRequiredSpecialization() const {
    return requiredSpecialization;
}

long Emergency::getTimestamp() const { return timestamp; }
