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
- Emergency intake & severity-based prioritization
- Ambulance availability tracking & nearest-match assignment
- Hospital matching by specialization & bed availability
- Optimal route calculation with dynamic re-routing
- Dispatch history logging & hospital load balancing

## Data Structures & Algorithms Used
| Feature | Data Structure / Algorithm |
|---|---|
| Emergency severity prioritization | Priority Queue (Heap) |
| Ambulance/hospital lookup | Hash Table |
| Road network representation | Graph (weighted adjacency list) |
| Route computation | Dijkstra's Algorithm / A* Search |
| Dispatch history & waitlist | Linked List |

## Tech Stack
- **Core Engine:** C++ (Graph, Priority Queue/Heap, Hash Table, Linked List, Dijkstra/A*)
- **Backend:** Node.js (Express) / Python (FastAPI)
- **Frontend:** React.js
- **Database:** MySQL / PostgreSQL
- **Maps:** Google Maps API / OpenStreetMap + Leaflet.js
- **Real-time:** WebSockets (Socket.io)

## Repository Structure
```
medroute/
├── core-engine/        # C++ DS/algorithm engine (Graph, Heap, HashTable, Dijkstra/A*)
│   ├── src/
│   ├── include/
│   └── tests/
├── backend/             # API layer connecting engine to frontend/DB
│   ├── routes/
│   └── controllers/
├── frontend/            # React dashboard
│   └── src/
├── database/            # Schema, migrations, seed data
│   └── schema.sql
├── docs/                 # Proposal, diagrams, presentations
└── README.md
```

## Project Status
🔵 Phase-I: Proposal & Design — *in progress*

## How to Run
*(To be added once initial modules are implemented)*
