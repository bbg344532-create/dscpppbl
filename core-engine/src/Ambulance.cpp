#include "Ambulance.h"

Ambulance::Ambulance(int id, int locationId)
    : id(id), currentLocationId(locationId), status(AmbulanceStatus::FREE) {}

int Ambulance::getId() const { return id; }

int Ambulance::getLocationId() const { return currentLocationId; }

AmbulanceStatus Ambulance::getStatus() const { return status; }

void Ambulance::setLocation(int locationId) { currentLocationId = locationId; }

void Ambulance::setStatus(AmbulanceStatus newStatus) { status = newStatus; }

bool Ambulance::isAvailable() const { return status == AmbulanceStatus::FREE; }

std::ostream& operator<<(std::ostream& os, const Ambulance& a) {
    return os << "Ambulance " << a.id << " [" << toString(a.status) << "] at node " << a.currentLocationId;
}
