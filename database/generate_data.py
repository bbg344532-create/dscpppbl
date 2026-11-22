#!/usr/bin/env python3
"""MedRoute data generator (Phase-III, Session 1).

Writes seed_data.sql and city.json next to this file. Output is deterministic
(fixed random seed), so every run produces identical files. All data is
invented demo data: no real places, hospitals or traffic.
"""
import heapq, json, math, os, random
from collections import deque

HERE = os.path.dirname(os.path.abspath(__file__))
rng = random.Random(2026)
ROWS, COLS = 5, 6

NAMES = ["Northgate Junction", "Harbour View", "Northside Cross", "Old Town Square", "Station Road", "Lakeview Point",
         "West End", "Maple Street", "Clock Tower", "Metro Plaza", "Market Circle", "East Depot Road",
         "Park Lane", "College Corner", "Central Junction", "Library Crossing", "Orchard Road", "Eastgate",
         "West Depot Road", "Canal Street", "Temple Road", "Riverside Drive", "Mill Street", "Garden Avenue",
         "Greenfield", "Bus Stand", "South Depot Road", "Fort Road", "Southside Cross", "Airport Road"]

def nid(r, c): return r * COLS + c + 1
def row(n): return (n - 1) // COLS
def col(n): return (n - 1) % COLS

# ---------- road network: 5 x 6 jittered grid, some roads removed, some diagonals added ----------
pos = {}
for r in range(ROWS):
    for c in range(COLS):
        pos[nid(r, c)] = (90 + c * 145 + rng.randint(-22, 22), 80 + r * 100 + rng.randint(-18, 18))

def connected(edges):
    adj = {n: [] for n in pos}
    for a, b in edges:
        adj[a].append(b); adj[b].append(a)
    seen, dq = {1}, deque([1])
    while dq:
        for m in adj[dq.popleft()]:
            if m not in seen: seen.add(m); dq.append(m)
    return len(seen) == len(pos)

edges = []
for r in range(ROWS):
    for c in range(COLS):
        if c + 1 < COLS: edges.append((nid(r, c), nid(r, c + 1)))
        if r + 1 < ROWS: edges.append((nid(r, c), nid(r + 1, c)))
order = edges[:]; rng.shuffle(order)
removed = 0
for e in order:
    if removed >= 11: break
    trial = [x for x in edges if x != e]
    if connected(trial): edges = trial; removed += 1
cells = [(r, c) for r in range(ROWS - 1) for c in range(COLS - 1)]; rng.shuffle(cells)
edges += [(nid(r, c), nid(r + 1, c + 1)) for r, c in cells[:7]]
assert connected(edges)

def weight(a, b):
    d = math.dist(pos[a], pos[b])
    arterial = (row(a) == row(b) == 2) or (col(a) == col(b) and col(a) in (2, 3))
    return max(1, round(d / 28 * (0.7 if arterial else 1.0)))
roads = sorted((a, b, weight(a, b)) for a, b in edges)

adj = {n: [] for n in pos}
for a, b, w in roads:
    adj[a].append((b, w)); adj[b].append((a, w))

def dijkstra(src):
    dist, pq = {src: 0}, [(0, src)]
    while pq:
        d, n = heapq.heappop(pq)
        if d > dist[n]: continue
        for m, w in adj[n]:
            if m not in dist or d + w < dist[m]: dist[m] = d + w; heapq.heappush(pq, (d + w, m))
    return dist

# ---------- entities ----------
EMERGENCY_TYPES = [("Cardiac Arrest", 9, "CARDIAC"), ("Severe Trauma", 8, "TRAUMA"), ("Accident", 8, "TRAUMA"),
    ("Stroke", 8, "GENERAL"), ("Breathing Difficulty", 7, "GENERAL"), ("Severe Bleeding", 7, "TRAUMA"),
    ("Child Emergency", 6, "PEDIATRIC"), ("Burns", 6, "TRAUMA"), ("Poisoning", 6, "GENERAL"),
    ("High Fever", 4, "GENERAL"), ("Fracture", 4, "GENERAL"), ("Minor Injury", 2, "GENERAL"), ("General Checkup", 1, "GENERAL")]

HOSPITALS = [  # id, name, node, total beds, available beds, specializations
    (1, "City General Hospital", 15, 40, 26, ["GENERAL", "TRAUMA"]),
    (2, "Metro Cardiac Centre", 10, 20, 11, ["CARDIAC"]),
    (3, "St. Mary's Children's Hospital", 8, 24, 15, ["PEDIATRIC", "GENERAL"]),
    (4, "Riverside Trauma Centre", 22, 16, 6, ["TRAUMA", "CARDIAC"]),
    (5, "Northside Community Hospital", 3, 30, 22, ["GENERAL"]),
    (6, "Lakeview Medical Centre", 6, 22, 12, ["GENERAL", "CARDIAC"]),
    (7, "Eastgate Emergency Hospital", 18, 18, 4, ["TRAUMA", "GENERAL"]),
    (8, "Greenfield Children's Clinic", 25, 10, 3, ["PEDIATRIC"]),
    (9, "Southside District Hospital", 29, 28, 17, ["GENERAL"])]
DEPOTS = [1, 12, 19, 27]
hosp_nodes = {h[2] for h in HOSPITALS}
assert len(hosp_nodes) == len(HOSPITALS) and not hosp_nodes & set(DEPOTS)

nodes = [(n, NAMES[n - 1], pos[n][0], pos[n][1],
          "HOSPITAL" if n in hosp_nodes else "DEPOT" if n in DEPOTS else "AREA") for n in sorted(pos)]

AMBULANCES = [(101, 1, "FREE"), (102, 12, "FREE"), (103, 19, "FREE"), (104, 27, "FREE"), (105, 1, "FREE"),
              (106, 12, "FREE"), (107, 14, "EN_ROUTE"), (108, 23, "EN_ROUTE"), (109, 9, "OCCUPIED")]

BASE_EMERGENCIES = [(21, "Cardiac Arrest", "2026-01-01 14:01:12"), (5, "Severe Bleeding", "2026-01-01 14:03:55"),
    (26, "Breathing Difficulty", "2026-01-01 14:05:02"), (13, "Child Emergency", "2026-01-01 14:06:18"),
    (30, "Fracture", "2026-01-01 14:07:40"), (9, "Minor Injury", "2026-01-01 14:09:05")]

# ---------- scenarios ----------
def only_free(free_id):  # every ambulance OCCUPIED except one, so the decision is easy to follow
    return [(a, loc, "FREE" if a == free_id else "OCCUPIED") for a, loc, _ in AMBULANCES]

d1, d27 = dijkstra(1), dijkstra(27)
def far(d): return sorted((n for n in d if n not in hosp_nodes), key=lambda n: (-d[n], n))[0]
def near(d): return sorted((n for n in d if n not in hosp_nodes and d[n] >= 1), key=lambda n: (d[n], n))[0]
assert d27[far(d27)] - d27[near(d27)] > 10 and d1[far(d1)] > 12

SCENARIOS = [
    (1, "Rush hour", "Eight emergencies reported together across the city; six ambulances are free. Shows ranking, top-5 evaluation and several matches in one cycle.",
     [(21, "Cardiac Arrest"), (16, "Accident"), (4, "Stroke"), (28, "Severe Bleeding"), (2, "Breathing Difficulty"),
      (13, "Child Emergency"), (24, "Burns"), (30, "Fracture")], [], [(a, loc, st) for a, loc, st in AMBULANCES]),
    (2, "Cardiac capacity exhausted", "All three cardiac-capable hospitals have 0 free beds. The Cardiac Arrest (severity 9) is skipped and the lone free ambulance serves another case.",
     [(21, "Cardiac Arrest"), (11, "Burns"), (14, "Fracture")], [(2, 0), (4, 0), (6, 0)], only_free(101)),
    (3, "Distance beats a small severity gap", "One free ambulance. A severity-8 Accident is far away; a severity-7 Breathing Difficulty is very close. The closer case wins.",
     [(far(d27), "Accident"), (near(d27), "Breathing Difficulty")], [], only_free(104)),
    (4, "Severity beats distance", "One free ambulance. A far-away Cardiac Arrest (severity 9) still outranks a Fracture right next to the ambulance.",
     [(near(d1), "Fracture"), (far(d1), "Cardiac Arrest")], [], only_free(101)),
]

# ---------- write seed_data.sql ----------
def q(v): return "'" + v.replace("'", "''") + "'" if isinstance(v, str) else str(v)
def insert(table, cols, rows):
    body = ",\n".join("(" + ", ".join(q(x) for x in r) + ")" for r in rows)
    return f"INSERT INTO {table} ({', '.join(cols)}) VALUES\n{body};\n\n"

sql = ["-- GENERATED by generate_data.py. Do not edit by hand; re-run the generator.\n",
       "-- Invented demo data: no real places, hospitals or traffic.\n\n"]
sql.append(insert("emergency_types", ["type", "severity", "required_specialization"], EMERGENCY_TYPES))
sql.append(insert("nodes", ["id", "name", "x", "y", "node_type"], nodes))
sql.append(insert("roads", ["from_node", "to_node", "weight", "base_weight"], [(a, b, w, w) for a, b, w in roads]))
sql.append(insert("hospitals", ["id", "name", "location_id", "total_beds", "available_beds"], [h[:5] for h in HOSPITALS]))
sql.append(insert("hospital_specializations", ["hospital_id", "specialization"], [(h[0], s) for h in HOSPITALS for s in h[5]]))
sql.append(insert("ambulances", ["id", "location_id", "status"], AMBULANCES))
sql.append(insert("emergencies", ["location_id", "emergency_type", "reported_at"], BASE_EMERGENCIES))
sql.append(insert("scenarios", ["id", "name", "description"], [(s[0], s[1], s[2]) for s in SCENARIOS]))
sql.append(insert("scenario_emergencies", ["scenario_id", "location_id", "emergency_type", "report_order"],
                  [(s[0], loc, t, i + 1) for s in SCENARIOS for i, (loc, t) in enumerate(s[3])]))
sql.append(insert("scenario_hospital_beds", ["scenario_id", "hospital_id", "available_beds"],
                  [(s[0], h, b) for s in SCENARIOS for h, b in s[4]]))
sql.append(insert("scenario_ambulances", ["scenario_id", "ambulance_id", "location_id", "status"],
                  [(s[0], a, loc, st) for s in SCENARIOS for a, loc, st in s[5]]))
open(os.path.join(HERE, "seed_data.sql"), "w").write("".join(sql))

# ---------- write city.json (copy for the backend/frontend in later sessions) ----------
city = {
    "nodes": [dict(id=n[0], name=n[1], x=n[2], y=n[3], type=n[4]) for n in nodes],
    "roads": [dict(from_node=a, to_node=b, weight=w) for a, b, w in roads],
    "emergency_types": [dict(type=t, severity=s, required_specialization=p) for t, s, p in EMERGENCY_TYPES],
    "hospitals": [dict(id=h[0], name=h[1], node=h[2], total_beds=h[3], available_beds=h[4], specializations=h[5]) for h in HOSPITALS],
    "ambulances": [dict(id=a, node=loc, status=st) for a, loc, st in AMBULANCES],
    "emergencies": [dict(node=loc, type=t, reported_at=ts) for loc, t, ts in BASE_EMERGENCIES],
    "scenarios": [dict(id=s[0], name=s[1], description=s[2],
                       emergencies=[dict(node=loc, type=t) for loc, t in s[3]],
                       hospital_beds=[dict(hospital_id=h, available_beds=b) for h, b in s[4]],
                       ambulances=[dict(id=a, node=loc, status=st) for a, loc, st in s[5]]) for s in SCENARIOS],
}
json.dump(city, open(os.path.join(HERE, "city.json"), "w"), indent=1)
print(f"Wrote seed_data.sql and city.json: {len(nodes)} nodes, {len(roads)} roads, {len(HOSPITALS)} hospitals, "
      f"{len(AMBULANCES)} ambulances, {len(SCENARIOS)} scenarios")
