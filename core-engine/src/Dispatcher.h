#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <vector>
#include <string>
#include "Graph.h"
#include "PriorityQueue.h"
#include "HashTable.h"
#include "LinkedList.h"
#include "SeverityTable.h"
#include "Ambulance.h"
#include "Hospital.h"
#include "Emergency.h"

// Dispatcher orchestrates the full MedRoute pipeline:
//   1. A reported emergency is scored via SeverityTable and pushed into the
//      PriorityQueue (global, not split by area/zone - see design notes).
//   2. When an ambulance becomes free, the Dispatcher does NOT blindly serve
//      the single highest-severity emergency. Instead it:
//        a) peeks at the top-K most severe waiting emergencies
//        b) filters out any with no currently FEASIBLE hospital
//           (specialization match + available beds)
//        c) among the feasible ones, scores by severity + distance from
//           this specific ambulance, and picks the best
//        d) if the best hospital candidate is a near-tie with another
//           feasible hospital, breaks the tie by hospital load (more free
//           beds wins) - this is the "load balancing" feature
//   3. The chosen emergency is removed from the queue, the ambulance is
//      assigned, the hospital's bed count is reserved, the route is computed
//      via Dijkstra, and the dispatch is logged to the LinkedList history.
//   4. Any emergency that had no feasible hospital at the time simply stays
//      in the Priority Queue - it is automatically reconsidered the next
//      time any ambulance frees up or any hospital's bed count changes.
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

    static constexpr int TOP_K = 5;
    static constexpr double SEVERITY_WEIGHT = 10.0;
    static constexpr double DISTANCE_WEIGHT = 1.0;
    static constexpr double HOSPITAL_LOAD_WEIGHT = 0.5;

    // Returns the best FEASIBLE hospital for this emergency, reachable from
    // 'fromLocationId' (the emergency's location). Returns nullptr if no
    // hospital currently qualifies (wrong specialization everywhere, or
    // every suitable hospital is full).
    Hospital* findBestFeasibleHospital(const Emergency& emergency, int fromLocationId) const;

    // Attempts to match 'ambulance' against the current waiting emergencies.
    // Returns true if a match was made (and performs the full assignment:
    // removes the emergency from the queue, updates ambulance/hospital state,
    // computes the route, and logs the dispatch).
    bool tryMatch(Ambulance* ambulance);

public:
    explicit Dispatcher(Graph& roadNetwork);
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

    int pendingCount() const;
    void printStatus() const;
    void printHistory() const;
};

#endif // DISPATCHER_H
