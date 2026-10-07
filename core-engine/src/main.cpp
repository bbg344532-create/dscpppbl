#include <iostream>
#include "Graph.h"
#include "Dispatcher.h"
#include "Ambulance.h"
#include "Hospital.h"

// Demo scenario: a small road network, a few hospitals, a few ambulances,
// and a handful of emergencies arriving over time - including the exact
// scenarios discussed during design (an ambulance should skip the
// top-priority emergency if no hospital can take it yet, and should prefer
// a closer, comparably urgent emergency over a far-away top-priority one).

int main() {
    std::cout << "=========================================\n";
    std::cout << " MedRoute - Emergency Routing Engine\n";
    std::cout << " Core DS/Algorithm Module\n";
    std::cout << "=========================================\n\n";

    // --- Build a small road network ---
    // Nodes 1-10 represent locations. Edge weights = travel distance/time.
    Graph graph;
    graph.addEdge(1, 2, 4);
    graph.addEdge(1, 3, 2);
    graph.addEdge(2, 4, 5);
    graph.addEdge(3, 4, 1);
    graph.addEdge(3, 5, 7);
    graph.addEdge(4, 6, 3);
    graph.addEdge(5, 6, 2);
    graph.addEdge(5, 7, 4);
    graph.addEdge(6, 8, 6);
    graph.addEdge(7, 8, 1);
    graph.addEdge(7, 9, 5);
    graph.addEdge(8, 10, 2);
    graph.addEdge(9, 10, 3);

    Dispatcher dispatcher(graph);

    // --- Hospitals ---
    Hospital* cityGeneral = new Hospital(1, 8, 2);   // only 2 beds - will fill up fast
    cityGeneral->addSpecialization(Specialization::GENERAL);
    cityGeneral->addSpecialization(Specialization::TRAUMA);

    Hospital* cardiacCenter = new Hospital(2, 10, 3);
    cardiacCenter->addSpecialization(Specialization::CARDIAC);

    Hospital* childrensHospital = new Hospital(3, 4, 2);
    childrensHospital->addSpecialization(Specialization::PEDIATRIC);
    childrensHospital->addSpecialization(Specialization::GENERAL);

    dispatcher.addHospital(cityGeneral);
    dispatcher.addHospital(cardiacCenter);
    dispatcher.addHospital(childrensHospital);

    // --- Ambulances ---
    Ambulance* amb1 = new Ambulance(101, 1);
    Ambulance* amb2 = new Ambulance(102, 9);
    dispatcher.addAmbulance(amb1);
    dispatcher.addAmbulance(amb2);

    std::cout << "--- Initial Status ---\n";
    dispatcher.printStatus();
    std::cout << "\n";

    // --- Scenario ---
    // Emergency 1: high severity (Cardiac Arrest), but reported at a location
    // far from both ambulances AND initially no hospital has cardiac beds free
    // until we add one - demonstrates feasibility-skipping if it occurs.
    std::cout << "--- Emergency 1 reported ---\n";
    dispatcher.reportEmergency(1, 7, "Cardiac Arrest", Specialization::CARDIAC);
    std::cout << "\n";

    // Emergency 2: lower severity, but very close to ambulance 102's position.
    std::cout << "--- Emergency 2 reported ---\n";
    dispatcher.reportEmergency(2, 10, "Fracture", Specialization::GENERAL);
    std::cout << "\n";

    std::cout << "--- Status after Emergency 1 & 2 ---\n";
    dispatcher.printStatus();
    std::cout << "\n";

    // Emergency 3: General, near city general hospital (which only has 2 beds).
    std::cout << "--- Emergency 3 reported ---\n";
    dispatcher.reportEmergency(3, 6, "Severe Bleeding", Specialization::TRAUMA);
    std::cout << "\n";

    // At this point both ambulances should be busy (EN_ROUTE), and emergency 3
    // (if it couldn't be matched - no free ambulance) stays in the queue.
    std::cout << "--- Status after Emergency 3 ---\n";
    dispatcher.printStatus();
    std::cout << "\n";

    // --- Ambulance 101 finishes its trip and becomes free again ---
    std::cout << "--- Ambulance 101 completes its dispatch ---\n";
    dispatcher.completeDispatch(101);
    std::cout << "\n";

    std::cout << "--- Final Status ---\n";
    dispatcher.printStatus();
    std::cout << "\n";

    dispatcher.printHistory();
    std::cout << "\n";

    std::cout << "Remaining emergencies still waiting in queue: "
              << dispatcher.pendingCount() << "\n";

    // --- Cleanup ---
    delete cityGeneral;
    delete cardiacCenter;
    delete childrensHospital;
    delete amb1;
    delete amb2;

    // =====================================================================
    // SCENARIO 2: Feasibility skip
    // A higher-severity emergency (Cardiac Arrest) has NO hospital that can
    // treat it, so it must be skipped - the ambulance should instead serve a
    // lower-severity but currently-servable emergency, rather than sitting
    // idle or being incorrectly sent to the cardiac case.
    // =====================================================================
    std::cout << "\n=========================================\n";
    std::cout << " SCENARIO 2: Feasibility Skip\n";
    std::cout << "=========================================\n";
    {
        Graph g2;
        g2.addEdge(1, 2, 3);   // ambulance -> fracture emergency
        g2.addEdge(2, 4, 1);   // fracture emergency -> hospital
        g2.addEdge(1, 3, 5);   // ambulance -> cardiac emergency

        Dispatcher d2(g2);

        Hospital* generalOnly = new Hospital(10, 4, 2);
        generalOnly->addSpecialization(Specialization::GENERAL);
        d2.addHospital(generalOnly); // note: NO cardiac hospital exists in this scenario

        Ambulance* a2 = new Ambulance(201, 1);
        d2.addAmbulance(a2);

        std::cout << "(No hospital in this scenario can treat CARDIAC cases.)\n";
        d2.reportEmergency(401, 3, "Cardiac Arrest", Specialization::CARDIAC); // severity 9, infeasible
        d2.reportEmergency(402, 2, "Fracture", Specialization::GENERAL);       // severity 4, feasible

        std::cout << "\nExpected: Emergency 402 (lower severity, but servable) gets the ambulance;\n";
        std::cout << "          Emergency 401 (higher severity, but no feasible hospital) stays queued.\n\n";
        d2.printStatus();
        std::cout << "Pending in queue (should be 1 - the cardiac case): " << d2.pendingCount() << "\n";

        delete generalOnly;
        delete a2;
    }

    // =====================================================================
    // SCENARIO 3: Distance can outweigh a severity gap
    // Emergency C (severity 8) is far from the ambulance; Emergency D
    // (severity 7, slightly lower) is very close. Our combined score
    // (severity*10 - distance) should favor the closer, comparably-urgent
    // case over the farther higher-severity one.
    // =====================================================================
    std::cout << "\n=========================================\n";
    std::cout << " SCENARIO 3: Distance vs. Severity Tradeoff\n";
    std::cout << "=========================================\n";
    {
        Graph g3;
        // Directed edges on purpose: this keeps the two routes (ambulance->D->hospital
        // and ambulance->C->hospital) fully independent, with no shortcut through the
        // shared hospital node, so the distances below are exact and unambiguous.
        g3.addEdge(1, 2, 2, false);    // ambulance -> close emergency (D)
        g3.addEdge(1, 3, 20, false);   // ambulance -> far emergency (C)
        g3.addEdge(2, 4, 1, false);    // close emergency -> hospital
        g3.addEdge(3, 4, 1, false);    // far emergency -> same hospital

        Dispatcher d3(g3);

        Hospital* genHospital = new Hospital(20, 4, 5);
        genHospital->addSpecialization(Specialization::GENERAL);
        d3.addHospital(genHospital);

        Ambulance* a3 = new Ambulance(301, 1);
        // Keep the ambulance artificially busy until BOTH emergencies have been
        // reported, so the Dispatcher actually has two candidates to choose
        // between (otherwise the first one reported would be auto-dispatched
        // immediately, before the second even exists to compete with it).
        a3->setStatus(AmbulanceStatus::EN_ROUTE);
        d3.addAmbulance(a3);

        std::cout << "(Emergency C: severity 8, distance 20 | Emergency D: severity 7, distance 2)\n";
        std::cout << "Score C = 8*10 - 20 = 60   |   Score D = 7*10 - 2 = 68  -> D should win\n\n";

        d3.reportEmergency(501, 3, "Accident", Specialization::GENERAL);            // C: severity 8, far
        d3.reportEmergency(502, 2, "Breathing Difficulty", Specialization::GENERAL); // D: severity 7, close

        std::cout << "\nBoth emergencies are now queued, ambulance still busy. Freeing it now:\n";
        d3.completeDispatch(301); // ambulance becomes free -> must choose between C and D

        std::cout << "\n";
        d3.printStatus();
        d3.printHistory();

        delete genHospital;
        delete a3;
    }

    return 0;
}
