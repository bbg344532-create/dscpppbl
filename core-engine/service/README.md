# MedRoute service

The core engine as a command-line program. The backend runs it once per action.

It is **stateless**: each run rebuilds the engine from the saved state it is given, applies ONE action
with the real Dispatcher, and prints the result as JSON. The database (through the backend) owns the
data; this program only makes decisions. If an action fails, it prints an error and changes nothing.

## Build (from the `core-engine` folder, Windows or Linux)

```
g++ -std=c++11 -Isrc service/service_main.cpp src/Graph.cpp src/PriorityQueue.cpp src/LinkedList.cpp src/Ambulance.cpp src/Hospital.cpp src/Emergency.cpp src/Dijkstra.cpp src/HopCountPlanner.cpp src/SeverityTable.cpp src/Dispatcher.cpp -o medroute_service
```

## Run

```
medroute_service service/samples/tiny_run.txt
medroute_service < service/samples/rush_hour_run.txt
```

Read the state from a file given on the command line, or from standard input. The output is one line of JSON
(`*.output.json` files in `samples/` show it pretty-printed). The exit code is 0 whenever JSON was printed,
including `"ok":false` errors; a non-zero exit means the program could not even start (for example the file was not found).

## Input format

One item per line. Blank lines and lines starting with `#` are ignored.

| Line | Meaning |
|---|---|
| `TYPE <severity> <SPECIALIZATION> <name...>` | An emergency type, e.g. `TYPE 9 CARDIAC Cardiac Arrest` |
| `ROAD <from> <to> <cost>` | A two-way road |
| `HOSPITAL <id> <node> <totalBeds> <availableBeds> <SPEC,SPEC>` | A hospital; beds already in use are total minus available |
| `AMBULANCE <id> <node> <FREE\|EN_ROUTE\|OCCUPIED>` | An ambulance |
| `EMERGENCY <id> <node> <reportOrder> <type name...>` | A WAITING emergency; lower report order = reported earlier |
| `TRIP <ambulanceId> <emergencyId> <patientNode> <hospitalId>` | An ambulance already on its way (needed to complete or re-plan it) |
| `ACTION ...` | Exactly one action, below |

Specializations: `GENERAL`, `TRAUMA`, `CARDIAC`, `PEDIATRIC`.

| Action | What it does |
|---|---|
| `ACTION state` | Nothing; just returns the state |
| `ACTION run` | Matches free ambulances to waiting emergencies until no more matches are possible |
| `ACTION report <id> <node> <type name...>` | Reports a new emergency, then matches |
| `ACTION complete <ambulanceId>` | The ambulance finished its trip: moves it to the hospital, frees it, matches again |
| `ACTION discharge <hospitalId>` | Frees one bed (if any patient is admitted), matches again |
| `ACTION traffic <fromNode> <toNode> <newCost>` | Changes a road's cost and re-plans trips in progress |

## Output format

On success (`"ok":true`):

| Field | Contents |
|---|---|
| `action` | The action that ran |
| `log` | The engine's own printed lines (`[DISPATCH] ...`, `[REPORTED] ...`) |
| `trace` | The Dispatcher's reasoning: each ambulance/emergency score, skipped cases, winners |
| `dispatches` | New trips created by this action: `emergencyId`, `ambulanceId`, `hospitalId`, `patientNode`, `pickupDistance`, `hospitalDistance`, `cost`, `route` |
| `completed` | Trips that ended with this action |
| `reroutes` | Trips whose route or cost changed (traffic): old and new cost and route |
| `roadChanges` | The road changed by a traffic action |
| `ambulances`, `hospitals` | The complete state after the action |
| `pending` | IDs of the emergencies still waiting, most urgent first |
| `trips` | All trips now in progress |

On failure: `{"ok":false,"error":{"type":"...","message":"..."}}` with types `InputError`, `UnknownLocationError`,
`UnknownIdError`, `UnknownRoadError`, `DuplicateIdError`, `InvalidArgumentError` or `InternalError`.

## Example

`samples/tiny_run.txt` has two free ambulances (at node 1 and node 3) and one waiting emergency at node 2.
The ambulance at node 1 is 1 away and the one at node 3 is 2 away, so 101 is sent.

```
{
 "ok": true,
 "action": "run",
 "log": [
  "[DISPATCH] Emergency 1 (severity 4) -> Ambulance 101 -> Hospital 1  | pickup dist=1, hospital dist=2"
 ],
 "trace": [
  "AMB-101 / EMG-1: severity 4, distance 1, score 39",
  "AMB-102 / EMG-1: severity 4, distance 2, score 38",
  "=> AMB-101 takes EMG-1, hospital 1 (score 39)"
 ],
 "dispatches": [
  {
   "ambulanceId": 101,
   "emergencyId": 1,
   "patientNode": 2,
   "hospitalId": 1,
   "pickupDistance": 1,
   "hospitalDistance": 2,
   "cost": 3,
   "route": [
    1,
    2,
    3
   ]
  }
 ],
 "completed": [],
 "reroutes": [],
 "roadChanges": [],
 "ambulances": [
  {
   "id": 101,
   "node": 1,
   "status": "EN_ROUTE"
  },
  {
   "id": 102,
   "node": 3,
   "status": "FREE"
  }
 ],
 "hospitals": [
  {
   "id": 1,
   "totalBeds": 10,
   "availableBeds": 5
  }
 ],
 "pending": [],
 "trips": [
  {
   "ambulanceId": 101,
   "emergencyId": 1,
   "patientNode": 2,
   "hospitalId": 1,
   "pickupDistance": 1,
   "hospitalDistance": 2,
   "cost": 3,
   "route": [
    1,
    2,
    3
   ]
  }
 ]
}
```

## Checking it

`cross_check.py` replays the default state and every demo scenario in `database/city.json` through the service
(run, complete, report, traffic, discharge, each fed the service's own previous output) and compares every answer with an
independent Python model of the rules. It also checks the error cases.

```
python service/cross_check.py medroute_service ../database/city.json
```
