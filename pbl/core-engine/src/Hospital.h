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
    int availableBeds;

public:
    Hospital(int id, int locationId, int availableBeds);

    int getId() const;
    int getLocationId() const;
    int getAvailableBeds() const;

    void addSpecialization(Specialization spec);
    bool canTreat(Specialization required) const;

    void admitPatient();   // decreases available beds
    void dischargePatient(); // increases available beds
};

#endif // HOSPITAL_H
