# MedRoute — Emergency Hospital Routing & Resource Optimization System

**Team ID:** DSCPP-III-2026-T195
**Mentor:** Ms. Meenakshi Maindola
**Semester:** 3rd | **Section:** C, D, I, K

## Team Members
| Name | Roll No. | Role |
|---|---|---|
| Rashi Kamboj | 2028027 | Team Lead / Backend & Integration |
| Daksh Kandari | 2027732 | Developer / Core DS-Algorithm Engine (C++) |
| Tanishka Joshi | 2029091 | Frontend & UI/UX |
| Aakarsh Gupta | 2027501 | Database & Documentation |

---

## Problem Statement

During a medical emergency, every minute matters — but the current process for getting help is largely manual and inefficient. Callers often don't know which nearby hospital can treat a specific emergency, ambulance dispatch is rarely based on true proximity/availability, and routes ignore real-time traffic conditions. **MedRoute** is a prototype system that prioritizes incoming emergencies by severity, matches each one to the nearest available and suitable ambulance, computes optimal traffic-aware routes, and selects the best hospital based on specialization and bed availability.

## Key Features
- Structured emergency intake (location + type) with automatic severity/specialization resolution
- Severity-based prioritization — most critical emergency served first, not first-come-first-served
- Feasibility-aware ambulance/hospital matching — skips emergencies with no currently available hospital rather than blocking the ambulance
- Combined severity + distance scoring — a nearby, moderately urgent case can be served before a far-away top-priority one
- Hospital load balancing across equally-suitable hospitals
- Dynamic re-routing support and full dispatch history logging

## Data Structures & Algorithms Used
| Feature | Data Structure / Algorithm |
|---|---|
| Emergency severity prioritization | Priority Queue (Max-Heap) |
| Ambulance/hospital/severity lookup | Hash Table (generic, templated on key + value type) |
| Road network representation | Graph (weighted adjacency list) |
| Route & distance computation | Dijkstra's Algorithm |
| Dispatch history & waitlist | Linked List |

## Tech Stack
- **Core Engine:** C++ (Graph, Priority Queue/Heap, Hash Table, Linked List, Dijkstra, Dispatcher)
- **Frontend:** React (Vite) — interactive dashboard, not yet wired to backend/DB
- **Backend:** Node.js (Express) / Python (FastAPI) — scaffolded, integration pending
- **Database:** MySQL / PostgreSQL — schema + seed data ready
- **Maps/Routing (future):** Google Maps API / OpenStreetMap + Leaflet.js
- **Real-time (future):** WebSockets (Socket.io)

## Repository Structure
```
medroute/
├── core-engine/              # C++ DS/algorithm engine — 100% complete
│   ├── src/
│   │   ├── main.cpp          # demo: 3 scenarios proving the design works
│   │   ├── Graph.h/.cpp
│   │   ├── PriorityQueue.h/.cpp
│   │   ├── HashTable.h/.cpp
│   │   ├── LinkedList.h/.cpp
│   │   ├── Dijkstra.h/.cpp
│   │   ├── SeverityTable.h/.cpp
│   │   ├── Dispatcher.h/.cpp  # core matching/decision logic
│   │   ├── Ambulance.h/.cpp
│   │   ├── Hospital.h/.cpp
│   │   └── Emergency.h/.cpp
│   └── CMakeLists.txt
├── backend/                  # API layer — scaffolded, not yet integrated
│   ├── routes/
│   └── controllers/
├── frontend/                 # React (Vite) interactive dashboard
│   └── src/
│       ├── components/       # Dashboard, EmergencyList, AmbulanceStatus,
│       │                     # HospitalList, MapView, ReportEmergencyForm,
│       │                     # DispatchReasoning
│       ├── engine/           # graph.js, dijkstra.js, severityTable.js,
│       │                     # dispatcher.js — JS port of the core engine's
│       │                     # matching logic, so the dashboard runs the
│       │                     # real algorithm client-side
│       ├── data/             # mockData.js — mirrors database/seed_data.sql
│       └── styles/
├── database/                 # Schema + seed data — ready
│   ├── schema.sql
│   └── seed_data.sql
├── docs/                     # Proposal, diagrams, presentations
└── README.md
```

## Project Status

**Phase-II: Core engine 100% complete, database ready, frontend interactive demo ready**

- **Core engine** — fully implemented and verified: compiles with zero warnings (`-Wall -Wextra -Wpedantic`), zero memory errors/leaks under AddressSanitizer, demonstrated through 3 working scenarios (full pipeline, feasibility-skip, distance-vs-severity tradeoff)
- **Database** — schema and seed data written and verified against a live SQLite test run
- **Frontend** — interactive React dashboard: structured emergency intake form, a "Run Dispatch Cycle" action that runs the real matching algorithm (ported to JS, using a real Dijkstra over the same weighted graph as the C++ engine), a live reasoning panel showing the algorithm's actual decisions, and route highlighting on a schematic network map. **Not yet connected to the backend/database** — this is intentional per mentor guidance for this phase.
- **Backend integration** — not started; deferred to Phase-III

## How to Run

### Core engine (C++)
```bash
cd core-engine/src
g++ -std=c++17 -Wall -Wextra main.cpp Graph.cpp PriorityQueue.cpp LinkedList.cpp \
    Ambulance.cpp Hospital.cpp Emergency.cpp Dijkstra.cpp SeverityTable.cpp Dispatcher.cpp \
    -o medroute_engine
./medroute_engine
```
Or via CMake:
```bash
cd core-engine
mkdir build && cd build
cmake ..
make
./medroute_engine
```

### Frontend (React/Vite)
```bash
cd frontend
npm install
npm run dev
```
Open the printed localhost URL. Use the "Report emergency" form to add a new case, then click "Run dispatch cycle" to watch the Dispatcher's real reasoning and the resulting route appear on the map.

### Database
```bash
mysql -u root -p < database/schema.sql
mysql -u root -p < database/seed_data.sql
```
(or load both files into any MySQL/PostgreSQL-compatible client)
