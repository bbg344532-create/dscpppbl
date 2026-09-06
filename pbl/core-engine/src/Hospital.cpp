#include "Hospital.h"
#include <algorithm>

Hospital::Hospital(int id, int locationId, int availableBeds)
    : id(id), locationId(locationId), availableBeds(availableBeds) {}

int Hospital::getId() const { return id; }

int Hospital::getLocationId() const { return locationId; }

int Hospital::getAvailableBeds() const { return availableBeds; }

void Hospital::addSpecialization(Specialization spec) {
    specializations.push_back(spec);
}

bool Hospital::canTreat(Specialization required) const {
    return std::find(specializations.begin(), specializations.end(), required)
           != specializations.end();
}

void Hospital::admitPatient() {
    if (availableBeds > 0) {
        availableBeds--;
    }
}

void Hospital::dischargePatient() {
    availableBeds++;
}
