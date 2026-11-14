#ifndef AMBULANCE_H
#define AMBULANCE_H

#include <string>
#include <ostream>

enum class AmbulanceStatus { FREE, EN_ROUTE, OCCUPIED };

inline const char* toString(AmbulanceStatus s) {
    switch (s) {
        case AmbulanceStatus::FREE: return "FREE";
        case AmbulanceStatus::EN_ROUTE: return "EN_ROUTE";
        case AmbulanceStatus::OCCUPIED: return "OCCUPIED";
    }
    return "UNKNOWN";
}

// Represents a single ambulance unit.
class Ambulance {
private:
    int id;
    int currentLocationId; // node id in the road-network graph
    AmbulanceStatus status;

public:
    // Friend function: not a member, but allowed to read the private data for printing.
    friend std::ostream& operator<<(std::ostream& os, const Ambulance& a);

    Ambulance(int id, int locationId);

    int getId() const;
    int getLocationId() const;
    AmbulanceStatus getStatus() const;

    void setLocation(int locationId);
    void setStatus(AmbulanceStatus newStatus);

    bool isAvailable() const;
};

#endif // AMBULANCE_H
