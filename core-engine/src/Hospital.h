#ifndef HOSPITAL_H
#define HOSPITAL_H

#include <string>
#include <vector>

// Represents a hospital's specialization capabilities
enum class Specialization { GENERAL, TRAUMA, CARDIAC, PEDIATRIC };

class Hospital {
private:
    int id;
    int locationId; // node id in the road-network graph
    std::vector<Specialization> specializations;
    int totalBeds;     // fixed capacity
    int availableBeds; // currently free; bounded between 0 and totalBeds

public:
    // Hospital starts at full capacity (availableBeds == totalBeds).
    Hospital(int id, int locationId, int totalBeds);

    int getId() const;
    int getLocationId() const;
    int getTotalBeds() const;
    int getAvailableBeds() const;

    void addSpecialization(Specialization spec);
    bool canTreat(Specialization required) const;

    void admitPatient();      // decreases available beds (floor at 0)
    void dischargePatient();  // increases available beds (capped at totalBeds)
};

#endif // HOSPITAL_H
