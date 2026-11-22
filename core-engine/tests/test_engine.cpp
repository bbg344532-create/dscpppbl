// MedRoute core-engine tests. No framework needed: build and run this one program.
//   g++ -std=c++11 -I../src test_engine.cpp <every src/*.cpp except main.cpp> -o medroute_tests
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Graph.h"
#include "PriorityQueue.h"
#include "HashTable.h"
#include "LinkedList.h"
#include "Ambulance.h"
#include "Hospital.h"
#include "Emergency.h"
#include "Dijkstra.h"
#include "HopCountPlanner.h"
#include "SeverityTable.h"
#include "Dispatcher.h"
#include "Exceptions.h"

static int g_total = 0, g_failed = 0;
#define CHECK(cond) do { ++g_total; if (!(cond)) { ++g_failed; std::cout << "  FAIL line " << __LINE__ << ": " #cond "\n"; } } while (0)
#define CHECK_EQ(a, b) do { ++g_total; if (!((a) == (b))) { ++g_failed; std::cout << "  FAIL line " << __LINE__ << ": " #a " == " #b "\n"; } } while (0)
#define CHECK_HAS(text, part) do { ++g_total; if ((text).find(part) == std::string::npos) { ++g_failed; std::cout << "  FAIL line " << __LINE__ << ": output missing \"" << part << "\"\n"; } } while (0)
#define CHECK_NOT_HAS(text, part) do { ++g_total; if ((text).find(part) != std::string::npos) { ++g_failed; std::cout << "  FAIL line " << __LINE__ << ": output should not contain \"" << part << "\"\n"; } } while (0)
#define CHECK_THROWS(stmt, ExType) do { ++g_total; bool caught = false; try { stmt; } catch (const ExType&) { caught = true; } if (!caught) { ++g_failed; std::cout << "  FAIL line " << __LINE__ << ": expected " #ExType "\n"; } } while (0)
#define RUN(fn) do { std::cout << "[ RUN ] " #fn "\n"; fn(); } while (0)

// Captures what the engine prints so tests can inspect it.
class Capture {
    std::ostringstream buf; std::streambuf* old; bool active;
public:
    Capture() : old(std::cout.rdbuf(buf.rdbuf())), active(true) {}
    ~Capture() { if (active) std::cout.rdbuf(old); }
    std::string finish() { if (active) { std::cout.rdbuf(old); active = false; } return buf.str(); }
};

static unsigned lcgState = 12345;
static unsigned lcg() { lcgState = lcgState * 1103515245u + 12345u; return (lcgState >> 16) & 0x7fff; }

// ---------------------------------------------------------------- PriorityQueue
static void test_pq_orders_by_severity() {
    PriorityQueue pq;
    pq.push(EmergencyEntry(1, 5, 0)); pq.push(EmergencyEntry(2, 9, 1)); pq.push(EmergencyEntry(3, 7, 2));
    CHECK_EQ(pq.pop().emergencyId, 2); CHECK_EQ(pq.pop().emergencyId, 3); CHECK_EQ(pq.pop().emergencyId, 1);
    CHECK(pq.isEmpty());
}
static void test_pq_tie_goes_to_earlier_report() {
    PriorityQueue pq;
    pq.push(EmergencyEntry(1, 7, 5)); pq.push(EmergencyEntry(2, 7, 1));
    CHECK_EQ(pq.pop().emergencyId, 2);
}
static void test_pq_peek_and_top_k_do_not_remove() {
    PriorityQueue pq;
    for (int i = 1; i <= 5; ++i) pq.push(EmergencyEntry(i, i * 2, i));
    std::vector<EmergencyEntry> top = pq.peekTopK(3);
    CHECK_EQ(top.size(), 3u); CHECK_EQ(top[0].emergencyId, 5); CHECK_EQ(top[1].emergencyId, 4); CHECK_EQ(top[2].emergencyId, 3);
    CHECK_EQ(pq.size(), 5); CHECK_EQ(pq.peek().emergencyId, 5); CHECK_EQ(pq.peekTopK(99).size(), 5u);
}
static void test_pq_remove_by_id_keeps_order() {
    PriorityQueue pq;
    int sev[7] = {0, 3, 8, 5, 9, 1, 7};
    for (int i = 1; i <= 6; ++i) pq.push(EmergencyEntry(i, sev[i], i));
    CHECK(pq.removeById(3)); CHECK(!pq.removeById(999)); CHECK_EQ(pq.size(), 5);
    int expected[5] = {4, 2, 6, 1, 5};
    for (int i = 0; i < 5; ++i) CHECK_EQ(pq.pop().emergencyId, expected[i]);
}
static void test_pq_stress_pops_in_order() {
    PriorityQueue pq;
    for (int i = 0; i < 300; ++i) pq.push(EmergencyEntry(i, 1 + static_cast<int>(lcg() % 9), i));
    EmergencyEntry prev = pq.pop();
    bool sorted = true;
    while (!pq.isEmpty()) { EmergencyEntry next = pq.pop(); if (next > prev) sorted = false; prev = next; }
    CHECK(sorted);
}
static void test_pq_empty_and_operator() {
    PriorityQueue pq;
    CHECK_THROWS(pq.pop(), EmptyQueueError); CHECK_THROWS(pq.peek(), std::runtime_error);
    EmergencyEntry a(1, 9, 5), b(2, 4, 1), c(3, 9, 2);
    CHECK(a > b); CHECK(!(b > a)); CHECK(c > a); CHECK(!(a > a));
}

// ---------------------------------------------------------------- HashTable
static void test_hash_table_int() {
    HashTable<int, int> h(3); // tiny capacity forces many collisions
    for (int k = 0; k < 60; ++k) h.insert(k, k * 10);
    CHECK_EQ(h.size(), 60);
    bool allFound = true;
    for (int k = 0; k < 60; ++k) { int v = -1; if (!h.find(k, v) || v != k * 10) allFound = false; }
    CHECK(allFound);
    h.insert(5, -1); int v = 0;
    CHECK_EQ(h.size(), 60); CHECK(h.find(5, v)); CHECK_EQ(v, -1);
    CHECK(h.remove(5)); CHECK(!h.remove(5)); CHECK(!h.contains(5)); CHECK(h.contains(59)); CHECK(!h.contains(60)); CHECK_EQ(h.size(), 59);
}
static void test_hash_table_string_keys() {
    HashTable<std::string, int> t;
    t.insert("Cardiac Arrest", 9); t.insert("Fracture", 4);
    int v = 0;
    CHECK(t.find("Cardiac Arrest", v)); CHECK_EQ(v, 9);
    v = 123; CHECK(!t.find("Nope", v)); CHECK_EQ(v, 123); // a miss leaves the output untouched
}

// ---------------------------------------------------------------- LinkedList
static void test_linked_list_keeps_order() {
    LinkedList list;
    list.append(DispatchRecord(1, 10, 5, "0")); list.append(DispatchRecord(2, 11, 6, "3")); list.append(DispatchRecord(3, 12, 7, "9"));
    CHECK_EQ(list.size(), 3);
    Capture c; list.printAll(); std::string out = c.finish();
    CHECK_HAS(out, "Emergency 1 -> Ambulance 10 -> Hospital 5 @ t=0");
    CHECK(out.find("Emergency 1") < out.find("Emergency 2") && out.find("Emergency 2") < out.find("Emergency 3"));
}

// ---------------------------------------------------------------- Graph and planners
static Graph makeLine() { Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 1); g.addEdge(3, 4, 1); g.addEdge(1, 4, 5); return g; }

static void test_graph_basics() {
    Graph g; g.addEdge(1, 2, 3);
    CHECK(g.hasNode(1)); CHECK(g.hasNode(2)); CHECK(!g.hasNode(9));
    CHECK_EQ(g.getNeighbors(1).size(), 1u); CHECK_EQ(g.getNeighbors(2).size(), 1u); CHECK(g.getNeighbors(9).empty());
    Graph d; d.addEdge(1, 2, 4, false);
    CHECK(d.hasNode(2)); CHECK(d.getNeighbors(2).empty()); CHECK(d.hasEdge(1, 2)); CHECK(!d.hasEdge(2, 1));
    CHECK_EQ(d.getEdgeWeight(1, 2), 4.0); CHECK_EQ(d.getEdgeWeight(2, 1), -1.0); CHECK_EQ(d.getNumNodes(), 2);
    d.updateEdgeWeight(1, 2, 9); CHECK_EQ(d.getEdgeWeight(1, 2), 9.0);
}
static void test_dijkstra() {
    Graph g = makeLine();
    CHECK_EQ(Dijkstra::shortestDistanceTo(g, 1, 4), 3.0); CHECK_EQ(Dijkstra::shortestDistanceTo(g, 2, 2), 0.0);
    PathResult r = Dijkstra::shortestPath(g, 1, 4);
    std::vector<int> expect = {1, 2, 3, 4};
    CHECK(r.reachable); CHECK(r.path == expect); CHECK_EQ(r.totalDistance, 3.0);
    CHECK_EQ(Dijkstra::shortestDistances(g, 1).size(), 4u);
    g.updateEdgeWeight(2, 3, 10); g.updateEdgeWeight(3, 2, 10);   // jam the middle road
    CHECK_EQ(Dijkstra::shortestDistanceTo(g, 1, 4), 5.0);          // now the direct road wins
    std::vector<int> direct = {1, 4};
    CHECK(Dijkstra::shortestPath(g, 1, 4).path == direct);
    g.addEdge(8, 9, 1);                                            // separate component
    CHECK_EQ(Dijkstra::shortestDistanceTo(g, 1, 9), -1.0); CHECK(!Dijkstra::shortestPath(g, 1, 9).reachable);
}
static void test_hop_count_and_polymorphism() {
    Graph g = makeLine();
    DijkstraPlanner dj; HopCountPlanner hop;
    const RoutePlanner* planners[2] = { &dj, &hop };
    CHECK_EQ(planners[0]->distance(g, 1, 4), 3.0); CHECK_EQ(planners[1]->distance(g, 1, 4), 1.0);
    std::vector<int> hopRoute = {1, 4};
    CHECK(planners[1]->path(g, 1, 4).path == hopRoute);
    CHECK(planners[0]->name() != planners[1]->name());
    CHECK_EQ(hop.distance(g, 3, 3), 0.0);
    g.addEdge(8, 9, 1); CHECK_EQ(hop.distance(g, 1, 9), -1.0);
}

// ---------------------------------------------------------------- Small classes
static void test_severity_table() {
    SeverityTable t;
    CHECK_EQ(t.getSeverity("Cardiac Arrest"), 9); CHECK_EQ(t.getSeverity("General Checkup"), 1);
    CHECK_EQ(t.getSeverity("Alien Attack"), SeverityTable::DEFAULT_SEVERITY);
    t.setSeverity("Alien Attack", 10); CHECK_EQ(t.getSeverity("Alien Attack"), 10);
}
static void test_hospital_beds_and_printing() {
    Hospital h(7, 3, 2); h.addSpecialization(Specialization::TRAUMA);
    CHECK(h.canTreat(Specialization::TRAUMA)); CHECK(!h.canTreat(Specialization::CARDIAC));
    h.admitPatient(); h.admitPatient(); h.admitPatient();
    CHECK_EQ(h.getAvailableBeds(), 0);                    // never below zero
    h.dischargePatient(); h.dischargePatient(); h.dischargePatient();
    CHECK_EQ(h.getAvailableBeds(), 2);                    // never above total
    std::ostringstream os; os << h; CHECK_HAS(os.str(), "beds 2/2"); CHECK_HAS(os.str(), "TRAUMA");
}
static void test_ambulance_and_emergency() {
    Ambulance a(4, 9); CHECK(a.isAvailable());
    a.setStatus(AmbulanceStatus::EN_ROUTE); CHECK(!a.isAvailable());
    std::ostringstream os; os << a; CHECK_HAS(os.str(), "Ambulance 4 [EN_ROUTE] at node 9");
    Emergency e(2, 5, 8, Specialization::CARDIAC, 3);
    std::ostringstream es; es << e; CHECK_HAS(es.str(), "severity 8"); CHECK_HAS(es.str(), "CARDIAC");
}
static void test_exception_hierarchy() {
    CHECK_THROWS(throw UnknownLocationError(7), MedRouteError);
    CHECK_THROWS(throw UnknownLocationError(7), std::runtime_error);
    CHECK_THROWS(throw DuplicateIdError("ambulance", 3), std::exception);
    try { throw UnknownLocationError(7); } catch (const UnknownLocationError& e) { CHECK_EQ(e.getNode(), 7); }
}

// ---------------------------------------------------------------- Dispatcher
static Graph makeTrafficGraph() {   // two ways to reach node 3, plus a link between them
    Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 1); g.addEdge(5, 4, 2); g.addEdge(4, 3, 2); g.addEdge(1, 4, 6); return g;
}

static void test_dispatch_nearest_feasible() {
    Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 1);
    Hospital h(1, 3, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a1(1, 1), a2(2, 3);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a1); d.addAmbulance(&a2);
    Capture c; d.reportEmergency(1, 3, "Fracture", Specialization::GENERAL); std::string out = c.finish();
    CHECK(a2.getStatus() == AmbulanceStatus::EN_ROUTE); CHECK(a1.getStatus() == AmbulanceStatus::FREE);
    CHECK_EQ(h.getAvailableBeds(), 4); CHECK_HAS(out, "Ambulance 2 -> Hospital 1");
}
static void test_severity_first_when_distance_is_similar() {
    Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 1);
    Hospital h(1, 3, 5); h.addSpecialization(Specialization::GENERAL); h.addSpecialization(Specialization::CARDIAC);
    Ambulance a(1, 1); a.setStatus(AmbulanceStatus::EN_ROUTE);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c; d.reportEmergency(1, 2, "Fracture", Specialization::GENERAL);
    d.reportEmergency(2, 3, "Cardiac Arrest", Specialization::CARDIAC);
    d.completeDispatch(1); std::string out = c.finish();
    CHECK_HAS(out, "Emergency 2 (severity 9)"); CHECK_EQ(d.pendingCount(), 1);
}
static void test_feasibility_skip() {
    Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 1);
    Hospital h(1, 3, 5); h.addSpecialization(Specialization::GENERAL);          // no cardiac capability
    Ambulance a(1, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c; d.reportEmergency(1, 3, "Cardiac Arrest", Specialization::CARDIAC);
    CHECK_EQ(d.pendingCount(), 1); CHECK(a.getStatus() == AmbulanceStatus::FREE);
    d.reportEmergency(2, 2, "Fracture", Specialization::GENERAL); std::string out = c.finish();
    CHECK_HAS(out, "Emergency 2 (severity 4)"); CHECK_EQ(d.pendingCount(), 1);
}
static void test_distance_beats_small_severity_gap() {
    Graph g; g.addEdge(1, 2, 2, false); g.addEdge(1, 3, 20, false); g.addEdge(2, 4, 1, false); g.addEdge(3, 4, 1, false);
    Hospital h(1, 4, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a(1, 1); a.setStatus(AmbulanceStatus::EN_ROUTE);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c; d.reportEmergency(1, 3, "Accident", Specialization::GENERAL);               // severity 8, far
    d.reportEmergency(2, 2, "Breathing Difficulty", Specialization::GENERAL);               // severity 7, close
    d.completeDispatch(1); std::string out = c.finish();
    CHECK_HAS(out, "Emergency 2 (severity 7)"); CHECK_EQ(d.pendingCount(), 1);
}
static void test_waiting_emergency_served_after_completion() {
    Graph g; g.addEdge(1, 2, 1);
    Hospital h(1, 2, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a(1, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c; d.reportEmergency(1, 1, "Fracture", Specialization::GENERAL); d.reportEmergency(2, 1, "Fracture", Specialization::GENERAL);
    CHECK_EQ(d.pendingCount(), 1);
    d.completeDispatch(1); c.finish();
    CHECK_EQ(d.pendingCount(), 0); CHECK(a.getStatus() == AmbulanceStatus::EN_ROUTE); CHECK_EQ(a.getLocationId(), 2);
}
static void test_dispatcher_validation() {
    Graph g; g.addEdge(1, 2, 1);
    Hospital h(1, 2, 5), h2(1, 2, 5), hBad(9, 77, 5); Ambulance a(1, 1), a2(1, 1), aBad(2, 77);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    CHECK_THROWS(d.addAmbulance(&a2), DuplicateIdError); CHECK_THROWS(d.addHospital(&h2), DuplicateIdError);
    CHECK_THROWS(d.addAmbulance(&aBad), UnknownLocationError); CHECK_THROWS(d.addHospital(&hBad), UnknownLocationError);
    Capture c;
    CHECK_THROWS(d.reportEmergency(1, 99, "Fracture", Specialization::GENERAL), UnknownLocationError);
    d.reportEmergency(1, 1, "Fracture", Specialization::GENERAL);
    CHECK_THROWS(d.reportEmergency(1, 1, "Fracture", Specialization::GENERAL), DuplicateIdError);
    CHECK_THROWS(d.dischargePatient(42), UnknownIdError);
    c.finish();
}
static void test_discharge_unblocks_waiting_emergency() {
    Graph g; g.addEdge(1, 2, 1);
    Hospital h(1, 2, 1); h.addSpecialization(Specialization::GENERAL);          // a single bed
    Ambulance a1(1, 1), a2(2, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a1); d.addAmbulance(&a2);
    Capture c;
    d.reportEmergency(1, 1, "Fracture", Specialization::GENERAL);                // takes the only bed
    d.reportEmergency(2, 1, "Minor Injury", Specialization::GENERAL);            // free ambulance, but no bed
    CHECK_EQ(d.pendingCount(), 1); CHECK(a2.getStatus() == AmbulanceStatus::FREE);
    bool freed = d.dischargePatient(1); std::string out = c.finish();
    CHECK(freed); CHECK_EQ(d.pendingCount(), 0); CHECK(a2.getStatus() == AmbulanceStatus::EN_ROUTE);
    CHECK_HAS(out, "[DISCHARGE] Hospital 1"); CHECK_HAS(out, "Emergency 2");
}
static void test_discharge_with_no_patients() {
    Graph g; g.addEdge(1, 2, 1);
    Hospital h(1, 2, 3); h.addSpecialization(Specialization::GENERAL);
    Dispatcher d(g); d.addHospital(&h);
    Capture c; bool freed = d.dischargePatient(1); std::string out = c.finish();
    CHECK(!freed); CHECK_EQ(h.getAvailableBeds(), 3); CHECK_HAS(out, "no admitted patients");
}
static void test_timed_stay_frees_beds() {
    Graph g; g.addEdge(1, 2, 1);
    Hospital h(1, 2, 1); h.addSpecialization(Specialization::GENERAL);
    Ambulance a1(1, 1), a2(2, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a1); d.addAmbulance(&a2);
    d.setStayDuration(5);
    Capture c;
    d.reportEmergency(1, 1, "Fracture", Specialization::GENERAL); d.reportEmergency(2, 1, "Minor Injury", Specialization::GENERAL);
    long before = d.currentTime();
    d.advanceTime(1); CHECK_EQ(d.pendingCount(), 1);                              // stay not over yet
    d.advanceTime(10); std::string out = c.finish();
    CHECK_EQ(d.pendingCount(), 0); CHECK(d.currentTime() >= before + 11);  // dispatching the waiting case moves the clock on too CHECK_HAS(out, "[DISCHARGE]");
    CHECK_THROWS(d.advanceTime(-1), InvalidArgumentError); CHECK_THROWS(d.setStayDuration(-1), InvalidArgumentError);
}
static void test_traffic_changes_the_decision() {
    Graph g1 = makeTrafficGraph(), g2 = makeTrafficGraph();
    Hospital h1(1, 3, 5), h2(2, 3, 5); h1.addSpecialization(Specialization::GENERAL); h2.addSpecialization(Specialization::GENERAL);
    Ambulance a1(1, 1), b1(2, 5), a2(3, 1), b2(4, 5);
    Dispatcher clear(g1), jammed(g2);
    clear.addHospital(&h1); clear.addAmbulance(&a1); clear.addAmbulance(&b1);
    jammed.addHospital(&h2); jammed.addAmbulance(&a2); jammed.addAmbulance(&b2);
    Capture c;
    clear.reportEmergency(1, 3, "Fracture", Specialization::GENERAL);
    jammed.applyTrafficEvent(2, 3, 20);
    jammed.reportEmergency(1, 3, "Fracture", Specialization::GENERAL);
    c.finish();
    CHECK(a1.getStatus() == AmbulanceStatus::EN_ROUTE); CHECK(b1.getStatus() == AmbulanceStatus::FREE);   // clear road: the near one goes
    CHECK(a2.getStatus() == AmbulanceStatus::FREE);     CHECK(b2.getStatus() == AmbulanceStatus::EN_ROUTE); // jam: the other one goes
}
static void test_traffic_reroutes_active_trips() {
    Graph g = makeTrafficGraph();
    Hospital h(1, 5, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a(1, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c;
    d.reportEmergency(1, 3, "Fracture", Specialization::GENERAL);
    c.finish();
    { Capture c1; d.applyTrafficEvent(2, 3, 20); std::string out = c1.finish();
      CHECK_HAS(out, "[TRAFFIC] Road 2-3: cost 1 -> 20");
      CHECK_HAS(out, "[REROUTE] Ambulance 1: cost 6 -> 12, route 1-2-3-4-5 -> 1-4-3-4-5"); }
    { Capture c2; d.applyTrafficEvent(2, 3, 20); std::string out = c2.finish(); CHECK_NOT_HAS(out, "[REROUTE]"); }
    { Capture c3; d.applyTrafficEvent(2, 3, 1); std::string out = c3.finish(); CHECK_HAS(out, "route 1-4-3-4-5 -> 1-2-3-4-5"); }
    { Capture c4; d.completeDispatch(1); d.applyTrafficEvent(2, 3, 20); std::string out = c4.finish(); CHECK_NOT_HAS(out, "[REROUTE]"); }
}
static void test_traffic_validation() {
    Graph g = makeTrafficGraph(); Dispatcher d(g);
    CHECK_THROWS(d.applyTrafficEvent(1, 99, 5), UnknownRoadError);
    CHECK_THROWS(d.applyTrafficEvent(1, 2, 0), InvalidArgumentError);
    CHECK_THROWS(d.applyTrafficEvent(1, 2, -3), InvalidArgumentError);
    CHECK_EQ(g.getEdgeWeight(1, 2), 1.0);   // a rejected event changes nothing
}

static void test_restore_does_not_dispatch() {
    Graph g; g.addEdge(1, 2, 1);
    Hospital h(1, 2, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a(1, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c;
    d.restoreEmergency(5, 2, "Fracture", Specialization::GENERAL, 7);
    std::string quiet = c.finish();
    CHECK_EQ(quiet.size(), 0u);                                   // restoring prints nothing
    CHECK_EQ(d.pendingCount(), 1); CHECK(a.getStatus() == AmbulanceStatus::FREE);   // and dispatches nothing
    std::vector<EmergencyEntry> pending = d.getPendingEntries();
    CHECK_EQ(pending.size(), 1u); CHECK_EQ(pending[0].emergencyId, 5); CHECK_EQ(pending[0].waitTimeStamp, 7);
    CHECK(d.currentTime() >= 8);                                  // new reports sort after restored ones
    Capture c2; d.dispatchWaiting(); c2.finish();
    CHECK_EQ(d.pendingCount(), 0); CHECK(a.getStatus() == AmbulanceStatus::EN_ROUTE);
    CHECK_THROWS(d.restoreEmergency(5, 2, "Fracture", Specialization::GENERAL, 9), DuplicateIdError);
    CHECK_THROWS(d.restoreEmergency(6, 99, "Fracture", Specialization::GENERAL, 9), UnknownLocationError);
}
static void test_restore_trip_then_complete() {
    Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 2);
    Hospital h(1, 3, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a(1, 1); a.setStatus(AmbulanceStatus::EN_ROUTE);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    d.restoreTrip(1, 40, 2, 1);
    CHECK_EQ(d.getActiveTrips().size(), 1u);
    const ActiveTrip& trip = d.getActiveTrips()[0];
    CHECK_EQ(trip.emergencyId, 40); CHECK_EQ(trip.hospitalId, 1); CHECK_EQ(trip.pickupDistance, 1.0); CHECK_EQ(trip.hospitalDistance, 2.0);
    std::vector<int> expectedRoute = {1, 2, 3};
    CHECK(trip.route == expectedRoute); CHECK_EQ(trip.cost, 3.0);
    CHECK_THROWS(d.restoreTrip(9, 40, 2, 1), UnknownIdError); CHECK_THROWS(d.restoreTrip(1, 40, 2, 9), UnknownIdError);
    Capture c; d.completeDispatch(1); c.finish();
    CHECK(a.getStatus() == AmbulanceStatus::FREE); CHECK_EQ(a.getLocationId(), 3); CHECK(d.getActiveTrips().empty());
}
static void test_trace_and_trip_details() {
    Graph g; g.addEdge(1, 2, 1); g.addEdge(2, 3, 1);
    Hospital h(1, 3, 5); h.addSpecialization(Specialization::GENERAL);
    Ambulance a(1, 1);
    Dispatcher d(g); d.addHospital(&h); d.addAmbulance(&a);
    Capture c;
    d.reportEmergency(1, 3, "Cardiac Arrest", Specialization::CARDIAC);   // no cardiac hospital: skipped
    d.reportEmergency(2, 2, "Fracture", Specialization::GENERAL);
    c.finish();
    std::string all; for (std::size_t i = 0; i < d.getTrace().size(); ++i) all += d.getTrace()[i] + "\n";
    CHECK_HAS(all, "skip EMG-1 (severity 9): no hospital currently has CARDIAC capacity");
    CHECK_HAS(all, "AMB-1 / EMG-2: severity 4, distance 1, score 39"); CHECK_HAS(all, "=> AMB-1 takes EMG-2, hospital 1");
    CHECK_EQ(d.getActiveTrips().size(), 1u); CHECK_EQ(d.getActiveTrips()[0].emergencyId, 2);
    d.clearTrace(); CHECK(d.getTrace().empty());
}

int main() {
    RUN(test_pq_orders_by_severity); RUN(test_pq_tie_goes_to_earlier_report); RUN(test_pq_peek_and_top_k_do_not_remove);
    RUN(test_pq_remove_by_id_keeps_order); RUN(test_pq_stress_pops_in_order); RUN(test_pq_empty_and_operator);
    RUN(test_hash_table_int); RUN(test_hash_table_string_keys); RUN(test_linked_list_keeps_order);
    RUN(test_graph_basics); RUN(test_dijkstra); RUN(test_hop_count_and_polymorphism);
    RUN(test_severity_table); RUN(test_hospital_beds_and_printing); RUN(test_ambulance_and_emergency); RUN(test_exception_hierarchy);
    RUN(test_dispatch_nearest_feasible); RUN(test_severity_first_when_distance_is_similar); RUN(test_feasibility_skip);
    RUN(test_distance_beats_small_severity_gap); RUN(test_waiting_emergency_served_after_completion); RUN(test_dispatcher_validation);
    RUN(test_discharge_unblocks_waiting_emergency); RUN(test_discharge_with_no_patients); RUN(test_timed_stay_frees_beds);
    RUN(test_traffic_changes_the_decision); RUN(test_traffic_reroutes_active_trips); RUN(test_traffic_validation);
    RUN(test_restore_does_not_dispatch); RUN(test_restore_trip_then_complete); RUN(test_trace_and_trip_details);
    std::cout << "\n" << (g_total - g_failed) << " of " << g_total << " checks passed";
    if (g_failed) { std::cout << " (" << g_failed << " FAILED)\n"; return 1; }
    std::cout << ". ALL TESTS PASSED\n";
    return 0;
}
