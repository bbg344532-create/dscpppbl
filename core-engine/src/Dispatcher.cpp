#include "Dispatcher.h"
#include "Dijkstra.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <limits>

namespace {
// Joins two routes that share a node: [a ... x] + [x ... b] -> [a ... x ... b]
std::vector<int> joinRoute(const std::vector<int>& first, const std::vector<int>& second) {
    std::vector<int> joined = first;
    for (std::size_t i = 1; i < second.size(); ++i) joined.push_back(second[i]);
    return joined;
}
std::string routeText(const std::vector<int>& route) {
    std::ostringstream os;
    for (std::size_t i = 0; i < route.size(); ++i) os << (i > 0 ? "-" : "") << route[i];
    return os.str();
}
}

Dispatcher::Dispatcher(Graph& roadNetwork, const RoutePlanner* routePlanner)
    : graph(roadNetwork), ambulanceById(101), hospitalById(101),
      emergencyById(101), simulatedClock(0), defaultPlanner(),
      planner(routePlanner ? routePlanner : &defaultPlanner), stayDuration(0) {}

Dispatcher::~Dispatcher() {
    for (Emergency* e : allEmergenciesCreated) {
        delete e;
    }
}

void Dispatcher::addAmbulance(Ambulance* ambulance) {
    if (ambulanceById.contains(ambulance->getId())) throw DuplicateIdError("ambulance", ambulance->getId());
    if (!graph.hasNode(ambulance->getLocationId())) throw UnknownLocationError(ambulance->getLocationId());
    ambulanceById.insert(ambulance->getId(), ambulance);
    allAmbulances.push_back(ambulance);
}

void Dispatcher::addHospital(Hospital* hospital) {
    if (hospitalById.contains(hospital->getId())) throw DuplicateIdError("hospital", hospital->getId());
    if (!graph.hasNode(hospital->getLocationId())) throw UnknownLocationError(hospital->getLocationId());
    hospitalById.insert(hospital->getId(), hospital);
    allHospitals.push_back(hospital);
}

Hospital* Dispatcher::findBestFeasibleHospital(const Emergency& emergency, int fromLocationId) const {
    Hospital* best = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (Hospital* h : allHospitals) {
        bool suitable = h->canTreat(emergency.getRequiredSpecialization()) && h->getAvailableBeds() > 0;
        if (!suitable) continue;

        double distance = planner->distance(graph, fromLocationId, h->getLocationId());
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

bool Dispatcher::tryMatch() {
    std::vector<EmergencyEntry> candidates = pendingEmergencies.peekTopK(TOP_K);
    if (candidates.empty()) return false;

    // Feasibility does not depend on the ambulance, so work it out once per candidate.
    std::vector<Emergency*> emergencies(candidates.size(), nullptr);
    std::vector<Hospital*> hospitals(candidates.size(), nullptr);
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        Emergency* e = nullptr;
        if (!emergencyById.find(candidates[i].emergencyId, e)) continue; // shouldn't happen
        emergencies[i] = e;
        // nullptr = no hospital can take it right now: it stays in the queue, untouched
        hospitals[i] = findBestFeasibleHospital(*e, e->getLocationId());
        if (hospitals[i] == nullptr) {
            std::ostringstream os;
            os << "skip EMG-" << e->getId() << " (severity " << e->getSeverity() << "): no hospital currently has "
               << toString(e->getRequiredSpecialization()) << " capacity";
            addTrace(os.str());
        }
    }

    // Score every (free ambulance, feasible emergency) pair; the highest score wins.
    // Ties keep the earlier ambulance, then the more urgent emergency.
    Ambulance* ambulance = nullptr;
    int bestEmergencyId = -1;
    Emergency* bestEmergency = nullptr;
    Hospital* bestHospital = nullptr;
    double bestScore = -std::numeric_limits<double>::infinity();

    for (Ambulance* candidateAmbulance : allAmbulances) {
        if (!candidateAmbulance->isAvailable()) continue;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            if (hospitals[i] == nullptr) continue;
            double distanceToPatient = planner->distance(
                graph, candidateAmbulance->getLocationId(), emergencies[i]->getLocationId());
            if (distanceToPatient < 0) continue; // unreachable from this ambulance
            double score = (SEVERITY_WEIGHT * candidates[i].severity) - (DISTANCE_WEIGHT * distanceToPatient);
            {
                std::ostringstream os;
                os << "AMB-" << candidateAmbulance->getId() << " / EMG-" << emergencies[i]->getId()
                   << ": severity " << candidates[i].severity << ", distance " << distanceToPatient << ", score " << score;
                addTrace(os.str());
            }
            if (score > bestScore) {
                bestScore = score;
                ambulance = candidateAmbulance;
                bestEmergencyId = candidates[i].emergencyId;
                bestEmergency = emergencies[i];
                bestHospital = hospitals[i];
            }
        }
    }

    if (bestEmergencyId == -1) {
        return false; // nothing among the top-K can be served by any free ambulance right now
    }

    {
        std::ostringstream os;
        os << "=> AMB-" << ambulance->getId() << " takes EMG-" << bestEmergencyId << ", hospital " << bestHospital->getId()
           << " (score " << bestScore << ")";
        trace.push_back(os.str());
    }

    // --- Commit the match ---
    pendingEmergencies.removeById(bestEmergencyId);

    PathResult toPatient = planner->path(graph, ambulance->getLocationId(), bestEmergency->getLocationId());
    PathResult toHospital = planner->path(graph, bestEmergency->getLocationId(), bestHospital->getLocationId());

    bestHospital->admitPatient(); // reserve the bed now, at assignment time
    ambulance->setStatus(AmbulanceStatus::EN_ROUTE);
    assignedHospitalLocationByAmbulance.insert(ambulance->getId(), bestHospital->getLocationId());

    long dispatchTime = simulatedClock++;
    std::ostringstream timeText;
    timeText << dispatchTime;
    dispatchHistory.append(DispatchRecord(bestEmergencyId, ambulance->getId(),
                                           bestHospital->getId(), timeText.str()));

    std::cout << "[DISPATCH] Emergency " << bestEmergencyId
              << " (severity " << bestEmergency->getSeverity() << ") -> Ambulance "
              << ambulance->getId() << " -> Hospital " << bestHospital->getId()
              << "  | pickup dist=" << toPatient.totalDistance
              << ", hospital dist=" << toHospital.totalDistance << "\n";

    if (stayDuration > 0) pendingDischarges.push(DischargeEvent(dispatchTime + stayDuration, bestHospital->getId()));

    ActiveTrip trip;
    trip.ambulanceId = ambulance->getId();
    trip.pickupNode = ambulance->getLocationId();
    trip.patientNode = bestEmergency->getLocationId();
    trip.hospitalNode = bestHospital->getLocationId();
    trip.route = joinRoute(toPatient.path, toHospital.path);
    trip.cost = toPatient.totalDistance + toHospital.totalDistance;
    trip.emergencyId = bestEmergencyId;
    trip.hospitalId = bestHospital->getId();
    trip.pickupDistance = toPatient.totalDistance;
    trip.hospitalDistance = toHospital.totalDistance;
    activeTrips.push_back(trip);

    return true;
}

void Dispatcher::reportEmergency(int emergencyId, int locationId,
                                  const std::string& emergencyType,
                                  Specialization requiredSpec) {
    if (!graph.hasNode(locationId)) throw UnknownLocationError(locationId);
    if (emergencyById.contains(emergencyId)) throw DuplicateIdError("emergency", emergencyId);
    int severity = severityTable.getSeverity(emergencyType);
    long timestamp = simulatedClock++;

    Emergency* emergency = new Emergency(emergencyId, locationId, severity, requiredSpec, timestamp);
    emergencyById.insert(emergencyId, emergency);
    allEmergenciesCreated.push_back(emergency);
    pendingEmergencies.push(EmergencyEntry(emergencyId, severity, timestamp));

    std::cout << "[REPORTED] Emergency " << emergencyId << " (\"" << emergencyType
              << "\", severity " << severity << ") at location " << locationId << "\n";

    // Try to serve it immediately if any ambulance is already free.
    runMatchingCycle();
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
    activeTrips.erase(std::remove_if(activeTrips.begin(), activeTrips.end(),
                                     [ambulanceId](const ActiveTrip& t) { return t.ambulanceId == ambulanceId; }),
                      activeTrips.end());
    amb->setStatus(AmbulanceStatus::FREE);

    std::cout << "[FREE] Ambulance " << ambulanceId << " is now available again.\n";

    // This ambulance freeing up may let a previously-skipped (infeasible) or
    // newly-arrived emergency finally be served.
    runMatchingCycle();
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

const RoutePlanner& Dispatcher::getRoutePlanner() const {
    return *planner;
}

void Dispatcher::runMatchingCycle() {
    while (!pendingEmergencies.isEmpty() && tryMatch()) {
        // keep matching until no (free ambulance, feasible emergency) pair is left
    }
}

bool Dispatcher::freeBed(Hospital* hospital) {
    if (hospital->getAvailableBeds() >= hospital->getTotalBeds()) return false;
    hospital->dischargePatient();
    std::cout << "[DISCHARGE] Hospital " << hospital->getId() << ": patient discharged, free beds "
              << hospital->getAvailableBeds() << "/" << hospital->getTotalBeds() << "\n";
    return true;
}

void Dispatcher::setStayDuration(long ticks) {
    if (ticks < 0) throw InvalidArgumentError("Stay duration cannot be negative");
    stayDuration = ticks;
}

bool Dispatcher::dischargePatient(int hospitalId) {
    Hospital* hospital = nullptr;
    if (!hospitalById.find(hospitalId, hospital)) throw UnknownIdError("hospital", hospitalId);
    if (!freeBed(hospital)) {
        std::cout << "[DISCHARGE] Hospital " << hospitalId << ": no admitted patients to discharge\n";
        return false;
    }
    runMatchingCycle(); // a freed bed may let a waiting emergency be served
    return true;
}

void Dispatcher::advanceTime(long ticks) {
    if (ticks < 0) throw InvalidArgumentError("Time cannot move backwards");
    simulatedClock += ticks;
    std::cout << "[TIME] advanced by " << ticks << " (clock " << simulatedClock << ")\n";

    bool freedAny = false;
    while (!pendingDischarges.empty() && pendingDischarges.top().dueTime <= simulatedClock) {
        DischargeEvent due = pendingDischarges.top();
        pendingDischarges.pop();
        Hospital* hospital = nullptr;
        if (hospitalById.find(due.hospitalId, hospital) && freeBed(hospital)) freedAny = true;
    }
    if (freedAny) runMatchingCycle();
}

long Dispatcher::currentTime() const {
    return simulatedClock;
}

void Dispatcher::applyTrafficEvent(int fromNode, int toNode, double newCost) {
    if (newCost <= 0) throw InvalidArgumentError("Road cost must be positive");
    bool forward = graph.hasEdge(fromNode, toNode);
    bool backward = graph.hasEdge(toNode, fromNode);
    if (!forward && !backward) throw UnknownRoadError(fromNode, toNode);

    double oldCost = forward ? graph.getEdgeWeight(fromNode, toNode) : graph.getEdgeWeight(toNode, fromNode);
    if (forward) graph.updateEdgeWeight(fromNode, toNode, newCost);
    if (backward) graph.updateEdgeWeight(toNode, fromNode, newCost);
    std::cout << "[TRAFFIC] Road " << fromNode << "-" << toNode << ": cost " << oldCost << " -> " << newCost << "\n";

    // Re-plan every trip still in progress with the new road costs.
    for (std::size_t i = 0; i < activeTrips.size(); ++i) {
        ActiveTrip& trip = activeTrips[i];
        PathResult toPatient = planner->path(graph, trip.pickupNode, trip.patientNode);
        PathResult toHospital = planner->path(graph, trip.patientNode, trip.hospitalNode);
        if (!toPatient.reachable || !toHospital.reachable) {
            std::cout << "[REROUTE] Ambulance " << trip.ambulanceId << ": no route available\n";
            continue;
        }
        std::vector<int> newRoute = joinRoute(toPatient.path, toHospital.path);
        double newTripCost = toPatient.totalDistance + toHospital.totalDistance;
        if (newRoute == trip.route && newTripCost == trip.cost) continue; // unaffected
        std::cout << "[REROUTE] Ambulance " << trip.ambulanceId << ": cost " << trip.cost << " -> " << newTripCost;
        if (newRoute == trip.route) std::cout << ", same route " << routeText(newRoute) << "\n";
        else std::cout << ", route " << routeText(trip.route) << " -> " << routeText(newRoute) << "\n";
        trip.route = newRoute;
        trip.cost = newTripCost;
    }
}

void Dispatcher::addTrace(const std::string& line) {
    if (std::find(trace.begin(), trace.end(), line) == trace.end()) trace.push_back(line);
}

void Dispatcher::dispatchWaiting() {
    runMatchingCycle();
}

void Dispatcher::restoreEmergency(int emergencyId, int locationId, const std::string& emergencyType,
                                  Specialization requiredSpec, long timestamp) {
    if (!graph.hasNode(locationId)) throw UnknownLocationError(locationId);
    if (emergencyById.contains(emergencyId)) throw DuplicateIdError("emergency", emergencyId);
    int severity = severityTable.getSeverity(emergencyType);
    Emergency* emergency = new Emergency(emergencyId, locationId, severity, requiredSpec, timestamp);
    emergencyById.insert(emergencyId, emergency);
    allEmergenciesCreated.push_back(emergency);
    pendingEmergencies.push(EmergencyEntry(emergencyId, severity, timestamp));
    if (timestamp >= simulatedClock) simulatedClock = timestamp + 1; // new reports must sort after restored ones
}

void Dispatcher::restoreTrip(int ambulanceId, int emergencyId, int patientNode, int hospitalId) {
    Ambulance* ambulance = nullptr;
    Hospital* hospital = nullptr;
    if (!ambulanceById.find(ambulanceId, ambulance)) throw UnknownIdError("ambulance", ambulanceId);
    if (!hospitalById.find(hospitalId, hospital)) throw UnknownIdError("hospital", hospitalId);
    if (!graph.hasNode(patientNode)) throw UnknownLocationError(patientNode);

    PathResult toPatient = planner->path(graph, ambulance->getLocationId(), patientNode);
    PathResult toHospital = planner->path(graph, patientNode, hospital->getLocationId());
    if (!toPatient.reachable || !toHospital.reachable) {
        std::ostringstream os;
        os << "Cannot rebuild the trip of ambulance " << ambulanceId << ": no route";
        throw MedRouteError(os.str());
    }

    ActiveTrip trip;
    trip.ambulanceId = ambulanceId;
    trip.emergencyId = emergencyId;
    trip.pickupNode = ambulance->getLocationId();
    trip.patientNode = patientNode;
    trip.hospitalId = hospitalId;
    trip.hospitalNode = hospital->getLocationId();
    trip.pickupDistance = toPatient.totalDistance;
    trip.hospitalDistance = toHospital.totalDistance;
    trip.route = joinRoute(toPatient.path, toHospital.path);
    trip.cost = toPatient.totalDistance + toHospital.totalDistance;
    activeTrips.erase(std::remove_if(activeTrips.begin(), activeTrips.end(),
                                     [ambulanceId](const ActiveTrip& t) { return t.ambulanceId == ambulanceId; }),
                      activeTrips.end());
    activeTrips.push_back(trip);
    assignedHospitalLocationByAmbulance.insert(ambulanceId, hospital->getLocationId());
}

SeverityTable& Dispatcher::getSeverityTable() { return severityTable; }
const std::vector<ActiveTrip>& Dispatcher::getActiveTrips() const { return activeTrips; }
std::vector<EmergencyEntry> Dispatcher::getPendingEntries() const { return pendingEmergencies.peekTopK(pendingEmergencies.size()); }
const std::vector<std::string>& Dispatcher::getTrace() const { return trace; }
void Dispatcher::clearTrace() { trace.clear(); }
