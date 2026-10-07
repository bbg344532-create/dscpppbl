#include "Dispatcher.h"
#include "Dijkstra.h"
#include <iostream>
#include <limits>

Dispatcher::Dispatcher(Graph& roadNetwork)
    : graph(roadNetwork), ambulanceById(101), hospitalById(101),
      emergencyById(101), simulatedClock(0) {}

Dispatcher::~Dispatcher() {
    for (Emergency* e : allEmergenciesCreated) {
        delete e;
    }
}

void Dispatcher::addAmbulance(Ambulance* ambulance) {
    ambulanceById.insert(ambulance->getId(), ambulance);
    allAmbulances.push_back(ambulance);
}

void Dispatcher::addHospital(Hospital* hospital) {
    hospitalById.insert(hospital->getId(), hospital);
    allHospitals.push_back(hospital);
}

Hospital* Dispatcher::findBestFeasibleHospital(const Emergency& emergency, int fromLocationId) const {
    Hospital* best = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (Hospital* h : allHospitals) {
        bool suitable = h->canTreat(emergency.getRequiredSpecialization()) && h->getAvailableBeds() > 0;
        if (!suitable) continue;

        double distance = Dijkstra::shortestDistanceTo(graph, fromLocationId, h->getLocationId());
        if (distance < 0) continue; // unreachable - not feasible

        // Load balancing: prefer closer hospitals, but a hospital with more free
        // beds gets a bonus so patients aren't all funneled into one facility.
        double score = (-DISTANCE_WEIGHT * distance) + (HOSPITAL_LOAD_WEIGHT * h->getAvailableBeds());
        if (score > bestScore) {
            bestScore = score;
            best = h;
        }
    }
    return best;
}

bool Dispatcher::tryMatch(Ambulance* ambulance) {
    if (!ambulance->isAvailable()) return false;

    std::vector<EmergencyEntry> candidates = pendingEmergencies.peekTopK(TOP_K);
    if (candidates.empty()) return false;

    int bestEmergencyId = -1;
    Emergency* bestEmergency = nullptr;
    Hospital* bestHospital = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (const EmergencyEntry& candidate : candidates) {
        Emergency* emergency = nullptr;
        if (!emergencyById.find(candidate.emergencyId, emergency)) continue; // shouldn't happen

        // Feasibility check: skip this emergency (for now) if no hospital can take it.
        Hospital* hospital = findBestFeasibleHospital(*emergency, emergency->getLocationId());
        if (hospital == nullptr) {
            continue; // stays in the queue, untouched - not skipped permanently
        }

        double distanceToPatient = Dijkstra::shortestDistanceTo(
            graph, ambulance->getLocationId(), emergency->getLocationId());
        if (distanceToPatient < 0) continue; // unreachable from this ambulance

        double score = (SEVERITY_WEIGHT * candidate.severity) - (DISTANCE_WEIGHT * distanceToPatient);
        if (score > bestScore) {
            bestScore = score;
            bestEmergencyId = candidate.emergencyId;
            bestEmergency = emergency;
            bestHospital = hospital;
        }
    }

    if (bestEmergencyId == -1) {
        return false; // nothing among the top-K is currently feasible for this ambulance
    }

    // --- Commit the match ---
    pendingEmergencies.removeById(bestEmergencyId);

    PathResult toPatient = Dijkstra::shortestPath(graph, ambulance->getLocationId(), bestEmergency->getLocationId());
    PathResult toHospital = Dijkstra::shortestPath(graph, bestEmergency->getLocationId(), bestHospital->getLocationId());

    bestHospital->admitPatient(); // reserve the bed now, at assignment time
    ambulance->setStatus(AmbulanceStatus::EN_ROUTE);
    assignedHospitalLocationByAmbulance.insert(ambulance->getId(), bestHospital->getLocationId());

    long dispatchTime = simulatedClock++;
    dispatchHistory.append(DispatchRecord(bestEmergencyId, ambulance->getId(),
                                           bestHospital->getId(), std::to_string(dispatchTime)));

    std::cout << "[DISPATCH] Emergency " << bestEmergencyId
              << " (severity " << bestEmergency->getSeverity() << ") -> Ambulance "
              << ambulance->getId() << " -> Hospital " << bestHospital->getId()
              << "  | pickup dist=" << toPatient.totalDistance
              << ", hospital dist=" << toHospital.totalDistance << "\n";

    return true;
}

void Dispatcher::reportEmergency(int emergencyId, int locationId,
                                  const std::string& emergencyType,
                                  Specialization requiredSpec) {
    int severity = severityTable.getSeverity(emergencyType);
    long timestamp = simulatedClock++;

    Emergency* emergency = new Emergency(emergencyId, locationId, severity, requiredSpec, timestamp);
    emergencyById.insert(emergencyId, emergency);
    allEmergenciesCreated.push_back(emergency);
    pendingEmergencies.push(EmergencyEntry(emergencyId, severity, timestamp));

    std::cout << "[REPORTED] Emergency " << emergencyId << " (\"" << emergencyType
              << "\", severity " << severity << ") at location " << locationId << "\n";

    // Try to serve it immediately if any ambulance is already free.
    bool matched = true;
    while (matched) {
        matched = false;
        if (pendingEmergencies.isEmpty()) break;
        for (Ambulance* amb : allAmbulances) {
            if (amb->isAvailable() && tryMatch(amb)) {
                matched = true;
                break;
            }
        }
    }
}

void Dispatcher::completeDispatch(int ambulanceId) {
    Ambulance* amb = nullptr;
    if (!ambulanceById.find(ambulanceId, amb)) {
        std::cout << "[WARN] completeDispatch called for unknown ambulance " << ambulanceId << "\n";
        return;
    }

    int hospitalLocation;
    if (assignedHospitalLocationByAmbulance.find(ambulanceId, hospitalLocation)) {
        amb->setLocation(hospitalLocation);
        assignedHospitalLocationByAmbulance.remove(ambulanceId);
    }
    amb->setStatus(AmbulanceStatus::FREE);

    std::cout << "[FREE] Ambulance " << ambulanceId << " is now available again.\n";

    // This ambulance freeing up may let a previously-skipped (infeasible) or
    // newly-arrived emergency finally be served.
    bool matched = true;
    while (matched) {
        matched = false;
        if (pendingEmergencies.isEmpty()) break;
        for (Ambulance* a : allAmbulances) {
            if (a->isAvailable() && tryMatch(a)) {
                matched = true;
                break;
            }
        }
    }
}

int Dispatcher::pendingCount() const {
    return pendingEmergencies.size();
}

void Dispatcher::printStatus() const {
    std::cout << "--- Status ---\n";
    std::cout << "Pending emergencies in queue: " << pendingEmergencies.size() << "\n";
    std::cout << "Ambulances:\n";
    for (Ambulance* a : allAmbulances) {
        std::string statusStr = a->getStatus() == AmbulanceStatus::FREE ? "FREE"
                               : a->getStatus() == AmbulanceStatus::EN_ROUTE ? "EN_ROUTE" : "OCCUPIED";
        std::cout << "  Ambulance " << a->getId() << " @ loc " << a->getLocationId()
                  << " - " << statusStr << "\n";
    }
    std::cout << "Hospitals:\n";
    for (Hospital* h : allHospitals) {
        std::cout << "  Hospital " << h->getId() << " @ loc " << h->getLocationId()
                  << " - beds available: " << h->getAvailableBeds() << "\n";
    }
}

void Dispatcher::printHistory() const {
    std::cout << "--- Dispatch History ---\n";
    dispatchHistory.printAll();
}
