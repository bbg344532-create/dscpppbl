# MedRoute — Emergency Hospital Routing & Resource Optimization System

**Team ID:** DSCPP-III-2026-T195
**Mentor:** Ms. Meenakshi Maindola
**Semester:** 3rd | **Section:** C, D, I, K
**Current Phase:** Phase-III — Full-Stack Integration

## Team Members
| Name | Roll No. | Role |
|---|---|---|
| Rashi Kamboj | 2028027 | Team Lead / Backend & Integration |
| Daksh Kandari | 2027732 | Developer / Core DS-Algorithm Engine (C++) |
| Tanishka Joshi | 2029091 | Frontend & UI/UX |
| Aakarsh Gupta | 2027501 | Database & Documentation |

---

## Problem Statement

During a medical emergency, every minute matters — but the current process for getting help is largely manual and inefficient. Callers often don't know which nearby hospital can treat a specific emergency, ambulance dispatch is rarely based on true proximity/availability, and routes ignore real-time traffic conditions. **MedRoute** prioritizes incoming emergencies by severity, matches each one to the nearest available and suitable ambulance, computes optimal traffic-aware routes, and selects the best hospital based on specialization and bed availability.

## What's New in Phase-III

Phase-II delivered the core engine, the database, and a standalone frontend demo. Phase-III connects them into one working system:

- Backend REST API connecting the frontend, the database, and the C++ engine
- Dispatch results and history persisted in the database instead of living only in browser memory
- Frontend dashboard reads and writes live data through the API (mock data removed from the main flow)
- Dispatch history and ambulance/hospital state survive page refreshes and restarts
- End-to-end testing and final documentation

## Key Features
- Structured emergency intake (location + type) with automatic severity/specialization resolution
- Severity-based prioritization — most critical emergency served first, not first-come-first-served
- Feasibility-aware ambulance/hospital matching — skips emergencies with no currently available hospital rather than blocking the ambulance
- Combined severity + distance scoring — a nearby, moderately urgent case can be served before a far-away top-priority one
- Hospital load balancing across equally-suitable hospitals
- Dynamic re-routing support and full dispatch history logging
- **(Phase-III)** Persistent storage of emergencies, ambulances, hospitals, and dispatch logs
- **(Phase-III)** API-driven dashboard with live status updates

## Data Structures & Algorithms Used
| Feature | Data Structure / Algorithm |
|---|---|
| Emergency severity prioritization | Priority Queue (Max-Heap) |
| Ambulance/hospital/severity lookup | Hash Table (generic, templated on key + value type) |
| Road network representation | Graph (weighted adjacency list) |
| Route & distance computation | Dijkstra's Algorithm |
| Dispatch history & waitlist | Linked List |

## System Architecture

```
┌──────────────┐    HTTP/JSON    ┌──────────────────┐    SQL     ┌────────────┐
│  Frontend    │ ◄─────────────► │  Backend (API)   │ ◄────────► │  Database  │
│  React/Vite  │                 │  Node + Express  │            │  MySQL     │
└──────────────┘                 └────────┬─────────┘            └────────────┘
                                          │ child process (JSON via stdin/stdout)
                                          ▼
                                 ┌──────────────────┐
                                 │  C++ Core Engine │
                                 │  Dispatcher,     │
                                 │  Dijkstra, Heap  │
                                 └──────────────────┘
```

1. Frontend submits an emergency to the backend.
2. Backend stores it in the database and loads current ambulance/hospital state.
3. Backend passes the state to the C++ engine, which runs the dispatch cycle.
4. Backend saves the dispatch decision and updated state, then returns the result.
5. Frontend displays the decision, reasoning, and route.

## Tech Stack
- **Core Engine:** C++ (Graph, Priority Queue/Heap, Hash Table, Linked List, Dijkstra, Dispatcher)
- **Frontend:** React (Vite) — interactive dashboard
- **Backend:** Node.js (Express)
- **Database:** MySQL
- **Maps/Routing (future):** Google Maps API / OpenStreetMap + Leaflet.js
- **Real-time (future):** WebSockets (Socket.io)

## Repository Structure
```
medroute/
├── core-engine/                # C++ DS/algorithm engine — complete
│   ├── src/
│   │   ├── main.cpp            # demo scenarios + CLI entry point for backend
│   │   ├── Graph.h/.cpp
│   │   ├── PriorityQueue.h/.cpp
│   │   ├── HashTable.h/.cpp
│   │   ├── LinkedList.h/.cpp
│   │   ├── Dijkstra.h/.cpp
│   │   ├── SeverityTable.h/.cpp
│   │   ├── Dispatcher.h/.cpp   # core matching/decision logic
│   │   ├── Ambulance.h/.cpp
│   │   ├── Hospital.h/.cpp
│   │   └── Emergency.h/.cpp
│   └── CMakeLists.txt
├── backend/                    # REST API layer — Phase-III focus
│   ├── server.js               # Express app entry point
│   ├── config/
│   │   └── db.js               # database connection
│   ├── routes/
│   │   ├── emergencies.js
│   │   ├── ambulances.js
│   │   ├── hospitals.js
│   │   └── dispatch.js
│   ├── controllers/
│   ├── services/
│   │   └── engineBridge.js     # runs the C++ engine and parses its output
│   ├── .env.example
│   └── package.json
├── frontend/                   # React (Vite) dashboard
│   └── src/
│       ├── components/         # Dashboard, EmergencyList, AmbulanceStatus,
│       │                       # HospitalList, MapView, ReportEmergencyForm,
│       │                       # DispatchReasoning
│       ├── api/                # API client (fetch wrappers for the backend)
│       ├── engine/             # JS port of the matching logic (offline demo mode)
│       ├── data/               # mockData.js (fallback/demo only)
│       └── styles/
├── database/                   # Schema + seed data
│   ├── schema.sql
│   └── seed_data.sql
├── docs/                       # Proposal, diagrams, presentations, reports
└── README.md
```

## API Endpoints

| Method | Endpoint | Description |
|---|---|---|
| GET | `/api/emergencies` | List all emergencies (filter by status) |
| POST | `/api/emergencies` | Report a new emergency (location + type) |
| GET | `/api/ambulances` | List ambulances and their current status |
| GET | `/api/hospitals` | List hospitals, specializations, and bed availability |
| POST | `/api/dispatch/run` | Run a dispatch cycle on pending emergencies |
| GET | `/api/dispatch/history` | Full dispatch history log |


### Future Scope
- Real map data via OpenStreetMap + Leaflet.js
- Live traffic-aware edge weights
- Real-time updates using WebSockets
- Authentication for dispatcher accounts

## Getting Started

### Prerequisites
- g++ with C++17 support (or CMake)
- Node.js 18+ and npm
- MySQL 8+

### 1. Database
```bash
mysql -u root -p < database/schema.sql
mysql -u root -p < database/seed_data.sql
```

### 2. Core engine (C++)
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

### 3. Backend
```bash
cd backend
cp .env.example .env     # then fill in your values
npm install
npm run dev
```

Example `.env`:
```
PORT=5000
DB_HOST=localhost
DB_USER=root
DB_PASSWORD=your_password
DB_NAME=medroute
ENGINE_PATH=../core-engine/build/medroute_engine
```

### 4. Frontend
```bash
cd frontend
npm install
npm run dev
```
Open the printed localhost URL. Use the "Report emergency" form to add a case, then click "Run dispatch cycle" to see the Dispatcher's reasoning and the resulting route on the map.

## Testing
- **Engine:** build with `-Wall -Wextra -Wpedantic` and run under AddressSanitizer
- **API:** test endpoints with Postman or curl
- **End-to-end:** report an emergency in the UI, run a dispatch cycle, and confirm the result appears in `/api/dispatch/history` and the database

## Mentor Guidance Log
| Phase | Focus | Status |
|---|---|---|
| Phase-I | Proposal, DS selection, design | Done |
| Phase-II | Core engine, database, frontend demo | Done |
| Phase-III | Backend integration, end-to-end system | Done |
