# MedRoute backend

A REST API that keeps all data in a SQLite file and asks the C++ engine to make every dispatch decision.

```
Frontend  ->  this backend (Express)  ->  medroute_service (the C++ engine)
                      |
                 SQLite file (backend/data/medroute.sqlite)
```

For every request that changes anything, the backend loads the current state from the database, runs the engine
once with that state and one action, and saves what the engine decided, all as one all-or-nothing change.
If the engine reports an error, nothing is saved. Requests that change data run one at a time.

## Set up (needs Node.js 18 or newer)

1. **Build the engine program** (from the `core-engine` folder; this produces `medroute_service`, or `medroute_service.exe` on Windows):

```
g++ -std=c++11 -Isrc service/service_main.cpp src/Graph.cpp src/PriorityQueue.cpp src/LinkedList.cpp src/Ambulance.cpp src/Hospital.cpp src/Emergency.cpp src/Dijkstra.cpp src/HopCountPlanner.cpp src/SeverityTable.cpp src/Dispatcher.cpp -o medroute_service
```

2. **Install and start** (from the `backend` folder):

```
npm install
npm start
```

It prints `MedRoute backend listening on port 5000`. The first start creates `backend/data/medroute.sqlite` from
`database/schema.sql` and `database/seed_data.sql`. Delete that file (or call `POST /api/reset`) to start over.

| Setting (environment variable) | Default |
|---|---|
| `PORT` | 5000 |
| `MEDROUTE_DB` | `backend/data/medroute.sqlite` |
| `MEDROUTE_ENGINE_PATH` | `core-engine/medroute_service` (`.exe` on Windows) |

## Endpoints

Reading:

| Request | Returns |
|---|---|
| `GET /api/health` | `{ok, database, engine}`; `engine` is false if the engine program was not found |
| `GET /api/state` | Everything: nodes, roads (current and base cost), emergency types, hospitals, ambulances, waiting emergencies (most urgent first), trips in progress, recent history, scenarios |
| `GET /api/history?limit=50` | Dispatch history, newest first |
| `GET /api/scenarios` | The ready-made demo scenarios |

Changing (each returns the engine's `log`, `trace` (its reasoning), `dispatches`, `completed`, `reroutes`, and the new `state`):

| Request | Body | What happens |
|---|---|---|
| `POST /api/emergencies` | `{"node": 11, "type": "Burns"}` | Reports an emergency (status 201, with `emergencyId`), then matches |
| `POST /api/dispatch/run` | none | Matches free ambulances to waiting emergencies |
| `POST /api/trips/:ambulanceId/complete` | none | The ambulance finished its trip; frees it and matches again |
| `POST /api/hospitals/:id/discharge` | none | Frees one bed and matches again |
| `POST /api/traffic` | `{"from": 3, "to": 4, "cost": 12}` | Sets a road's cost and re-plans trips in progress. Send the road's `baseCost` to clear the jam |
| `POST /api/scenarios/:id/load` | none | Resets the data and applies a demo scenario |
| `POST /api/reset` | none | Resets to the default starting data |

Errors look like `{"ok": false, "error": {"type": "...", "message": "..."}}`:

| Status | Meaning |
|---|---|
| 400 | Bad input (`BadRequest`, `UnknownLocationError`, `UnknownEmergencyType`, `UnknownRoadError`) |
| 404 | Unknown ID or route (`UnknownIdError`, `UnknownScenario`, `NotFound`) |
| 409 | Conflict (`NotEnRoute`, `DuplicateIdError`) |
| 503 | Engine program not found or could not start (`EngineUnavailable`); reading still works |
| 500/504 | Unexpected error / engine timed out |

## Try it (Windows Command Prompt, using curl)

```
curl http://localhost:5000/api/health
curl -X POST http://localhost:5000/api/dispatch/run
curl -X POST -H "Content-Type: application/json" -d "{\"node\":11,\"type\":\"Burns\"}" http://localhost:5000/api/emergencies
curl -X POST http://localhost:5000/api/scenarios/2/load
```

## Database notes

- Emergencies have a status: `WAITING` (in the queue), `DISPATCHED` (a trip is in progress), `COMPLETED`.
- A trip in progress is a `dispatch_history` row with `completed_at` empty. It stores the route and cost.
- `roads.weight` is the current cost; `roads.base_weight` is the normal cost.
- The schema file is written in MySQL style. The backend converts `AUTO_INCREMENT` for SQLite when it loads it. It has been run on SQLite only.
- `sql.js` is SQLite compiled to WebAssembly, so there is nothing native to build. It keeps the database in memory and writes the whole file after each change, which is fine for a prototype of this size.
- CORS is open to any origin, which suits a local prototype and should be tightened for anything public.

## Tests

```
npm test
```

12 end-to-end tests start the real server with a temporary database and the real engine: every endpoint, error
cases (nothing changes on an error), two reports at the same instant, a restart that keeps the data, a missing engine, the demo
scenarios, and a 50-action random session that checks after every step that beds stay within capacity, no ambulance has two trips,
and no dispatched emergency is also waiting. If the engine is not at the default path, set `MEDROUTE_ENGINE_PATH` first.
