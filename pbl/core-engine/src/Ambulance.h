#ifndef AMBULANCE_H
#define AMBULANCE_H

#include <string>

enum class AmbulanceStatus { FREE, EN_ROUTE, OCCUPIED };

// Represents a single ambulance unit.
class Ambulance {
private:
    int id;
    int currentLocationId; // node id in the road-network graph
    AmbulanceStatus status;

public:
    Ambulance(int id, int locationId);

    int getId() const;
    int getLocationId() const;
    AmbulanceStatus getStatus() const;

    void setLocation(int locationId);
    void setStatus(AmbulanceStatus newStatus);

    bool isAvailable() const;
};

#endif // AMBULANCE_H
