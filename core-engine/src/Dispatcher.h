#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include <string>
#include <queue>
#include <functional>
#include "Graph.h"
#include "PriorityQueue.h"
#include "HashTable.h"
#include "LinkedList.h"
#include "SeverityTable.h"
#include "Ambulance.h"
#include "Hospital.h"
#include "Emergency.h"
#include "RoutePlanner.h"
#include "Dijkstra.h"
#include "Exceptions.h"

// Dispatcher orchestrates the full MedRoute pipeline:
//   1. A reported emergency is scored via SeverityTable and pushed into the
//      PriorityQueue (global, not split by area/zone - see design notes).
//   2. Whenever there is a free ambulance and a waiting emergency, the
//      Dispatcher does NOT blindly serve the single highest-severity
//      emergency, and does NOT hand it to whichever ambulance is first in
//      the list. Instead it:
//        a) peeks at the top-K most severe waiting emergencies
//        b) filters out any with no currently FEASIBLE hospital
//           (specialization match + available beds)
//        c) scores EVERY (free ambulance, feasible emergency) pair as
//           severity x 10 - distance, and picks the best pair overall, so
//           the nearest free ambulance takes each emergency
//        d) chooses the hospital by distance, with a bonus for free beds
//           (load balancing)
//   3. The chosen emergency is removed from the queue, the ambulance is
//      assigned, the hospital's bed count is reserved, the route is computed
//      via Dijkstra, and the dispatch is logged to the LinkedList history.
//   4. Any emergency that had no feasible hospital at the time simply stays
//      in the Priority Queue - it is automatically reconsidered the next
//      time any ambulance frees up or any hospital's bed count changes.
// A scheduled bed release: patient leaves hospital 'hospitalId' at time 'dueTime'.
struct DischargeEvent {
    long dueTime;
    int hospitalId;
    DischargeEvent(long due, int hospital) : dueTime(due), hospitalId(hospital) {}
    bool operator>(const DischargeEvent& other) const { return dueTime > other.dueTime; }
};

// A dispatched trip that has not been completed yet. Ambulances have no live
// position in this model, so a trip is always planned from the node the
// ambulance started at: ambulance -> patient -> hospital.
struct ActiveTrip {
    int ambulanceId;
    int emergencyId;
    int pickupNode;
    int patientNode;
    int hospitalId;
    int hospitalNode;
    double pickupDistance;    // ambulance -> patient
    double hospitalDistance;  // patient -> hospital
    std::vector<int> route;   // full node sequence, start to hospital
    double cost;              // total route cost
    ActiveTrip() : ambulanceId(0), emergencyId(0), pickupNode(0), patientNode(0), hospitalId(0),
                   hospitalNode(0), pickupDistance(0), hospitalDistance(0), cost(0) {}
};

class Dispatcher {
private:
    Graph& graph;
    PriorityQueue pendingEmergencies;
    SeverityTable severityTable;
    LinkedList dispatchHistory;

    // Hash Tables give O(1) average lookup by ID...
    HashTable<int, Ambulance*> ambulanceById;
    HashTable<int, Hospital*> hospitalById;
    HashTable<int, Emergency*> emergencyById;

    // ...but the Dispatcher also needs to iterate "all ambulances" / "all
    // hospitals" to search for the nearest available one, so it keeps a
    // parallel list alongside each Hash Table purely for iteration.
    std::vector<Ambulance*> allAmbulances;
    std::vector<Hospital*> allHospitals;

    // Emergency objects are created (heap-allocated) by the Dispatcher itself
    // inside reportEmergency(), so the Dispatcher also owns their cleanup.
    std::vector<Emergency*> allEmergenciesCreated;

    // Tracks which hospital (by location) each EN_ROUTE ambulance is headed to,
    // so completeDispatch() knows where to move the ambulance once the trip finishes.
    HashTable<int, int> assignedHospitalLocationByAmbulance;

    long simulatedClock; // simple incrementing "timestamp" for the demo

    // Route planning goes through the RoutePlanner interface (virtual functions), so the
    // Dispatcher does not know or care which algorithm is behind it.
    DijkstraPlanner defaultPlanner;
    const RoutePlanner* planner; // not owned: points at defaultPlanner unless one is injected

    // --- Phase-III: bed discharge and traffic ---
    long stayDuration; // ticks a patient occupies a bed; 0 = beds are only freed by dischargePatient()
    std::priority_queue<DischargeEvent, std::vector<DischargeEvent>, std::greater<DischargeEvent> > pendingDischarges; // min-heap by due time
    std::vector<ActiveTrip> activeTrips;
    std::vector<std::string> trace; // the Dispatcher's reasoning: scores, skipped cases, winners
    void addTrace(const std::string& line); // adds a line unless it is already there

    // Keeps matching free ambulances to feasible emergencies until nothing more can be matched.
    void runMatchingCycle();
    // Frees one bed at a hospital and prints it. False if the hospital had no admitted patients.
    bool freeBed(Hospital* hospital);

    static constexpr int TOP_K = 5;
    static constexpr double SEVERITY_WEIGHT = 10.0;
    static constexpr double DISTANCE_WEIGHT = 1.0;
    static constexpr double HOSPITAL_LOAD_WEIGHT = 0.5;

    // Returns the best FEASIBLE hospital for this emergency, reachable from
    // 'fromLocationId' (the emergency's location). Returns nullptr if no
    // hospital currently qualifies (wrong specialization everywhere, or
    // every suitable hospital is full).
    Hospital* findBestFeasibleHospital(const Emergency& emergency, int fromLocationId) const;

    // Finds the best (free ambulance, waiting emergency) pair across all free
    // ambulances and the top-K emergencies, and commits it: removes the
    // emergency from the queue, updates ambulance/hospital state, computes
    // the route, and logs the dispatch. Returns false if no pair is possible.
    bool tryMatch();

public:
    // Uses Dijkstra unless a different RoutePlanner is passed in (it must outlive the Dispatcher).
    explicit Dispatcher(Graph& roadNetwork, const RoutePlanner* routePlanner = nullptr);
    ~Dispatcher(); // frees all Emergency objects it created

    void addAmbulance(Ambulance* ambulance);
    void addHospital(Hospital* hospital);

    // Reports a new emergency using a structured emergency TYPE (e.g. "Cardiac
    // Arrest"), not free text. Severity is looked up from SeverityTable.
    // If any ambulance is already free, this will immediately attempt to
    // dispatch one; otherwise the emergency waits in the Priority Queue.
    void reportEmergency(int emergencyId, int locationId,
                          const std::string& emergencyType,
                          Specialization requiredSpec);

    // Call this once a dispatched ambulance has finished its trip (dropped the
    // patient at the hospital). Frees the ambulance and immediately tries to
    // match it against the next best waiting emergency, if any.
    void completeDispatch(int ambulanceId);

    // Bed discharge. dischargePatient() frees one bed at once (false if nobody was admitted) and
    // re-runs matching. With setStayDuration(n) beds free themselves n ticks after admission,
    // as time is advanced with advanceTime(). A manual discharge does not cancel a timer.
    void setStayDuration(long ticks);
    bool dischargePatient(int hospitalId);
    void advanceTime(long ticks);
    long currentTime() const;

    // Traffic. Sets the cost of the road between two nodes (both directions where the road
    // exists), then re-plans every active trip and prints the ones whose route or cost changed.
    // Calling it again with the original cost clears the jam.
    void applyTrafficEvent(int fromNode, int toNode, double newCost);

    // Runs the matching cycle now (used after loading saved state, or to retry waiting emergencies).
    void dispatchWaiting();

    // Rebuilding a Dispatcher from saved state (used by the service program). These add data
    // without printing and without dispatching anything.
    void restoreEmergency(int emergencyId, int locationId, const std::string& emergencyType,
                          Specialization requiredSpec, long timestamp);
    void restoreTrip(int ambulanceId, int emergencyId, int patientNode, int hospitalId);

    // Read-only views of the current state.
    SeverityTable& getSeverityTable();
    const std::vector<ActiveTrip>& getActiveTrips() const;
    std::vector<EmergencyEntry> getPendingEntries() const; // waiting emergencies, most urgent first
    const std::vector<std::string>& getTrace() const;
    void clearTrace();

    const RoutePlanner& getRoutePlanner() const;
    int pendingCount() const;
    void printStatus() const;
    void printHistory() const;
};

#endif // DISPATCHER_H
