#!/usr/bin/env python3
"""MedRoute data checker (Phase-III, Session 1).

Loads schema.sql + seed_data.sql into an in-memory SQLite database (foreign
keys ON), validates the data, and replays every demo scenario through a
Python reference copy of Dispatcher::tryMatch to confirm each scenario shows
what its description claims. Exit code 1 if anything fails.
"""
import heapq, json, os, sqlite3, sys
from collections import deque

D = os.path.dirname(os.path.abspath(__file__))
fails = []
def ok(cond, msg):
    print(("PASS  " if cond else "FAIL  ") + msg)
    if not cond: fails.append(msg)

schema = open(f"{D}/schema.sql").read().replace("INT PRIMARY KEY AUTO_INCREMENT", "INTEGER PRIMARY KEY AUTOINCREMENT")
con = sqlite3.connect(":memory:")
con.execute("PRAGMA foreign_keys = ON")
con.executescript(schema)
con.executescript(open(f"{D}/seed_data.sql").read())
def q(sql, *args): return con.execute(sql, args).fetchall()
def one(sql, *args): return q(sql, *args)[0][0]

print("--- Loading and counts ---")
counts = {t: one(f"SELECT COUNT(*) FROM {t}") for t in
          ["nodes", "roads", "emergency_types", "hospitals", "hospital_specializations", "ambulances", "emergencies", "scenarios"]}
print("      " + ", ".join(f"{k}={v}" for k, v in counts.items()))
ok(counts["nodes"] >= 25 and counts["roads"] >= counts["nodes"], "graph has at least 25 nodes and enough roads")

print("--- Graph ---")
adj = {}
for a, b, w in q("SELECT from_node, to_node, weight FROM roads"):
    adj.setdefault(a, []).append((b, w)); adj.setdefault(b, []).append((a, w))
node_ids = {r[0] for r in q("SELECT id FROM nodes")}
seen, dq = {min(node_ids)}, deque([min(node_ids)])
while dq:
    for m, _ in adj.get(dq.popleft(), []):
        if m not in seen: seen.add(m); dq.append(m)
ok(seen == node_ids, "road network is connected (every node reachable from every other)")
ok(one("SELECT COUNT(*) FROM roads WHERE base_weight != weight") == 0 and one("SELECT COUNT(*) FROM emergencies WHERE status != 'WAITING'") == 0, "seed starts clean: base costs equal current costs, every emergency WAITING")
ok(one("SELECT COUNT(*) FROM roads WHERE weight <= 0 OR from_node >= to_node") == 0, "all weights positive, no self-loops, each road stored once")
ok(all(n in adj for n in node_ids), "no isolated nodes")

print("--- Integrity ---")
ok(q("PRAGMA foreign_key_check") == [], "no orphaned foreign keys anywhere")
ok(one("SELECT COUNT(*) FROM hospitals WHERE available_beds > total_beds OR available_beds < 0") == 0, "bed counts within 0..total")
ok(one("SELECT COUNT(*) FROM hospitals h WHERE NOT EXISTS (SELECT 1 FROM hospital_specializations s WHERE s.hospital_id = h.id)") == 0, "every hospital has a specialization")
ok(one("SELECT COUNT(*) FROM emergency_types t WHERE NOT EXISTS (SELECT 1 FROM hospital_specializations s WHERE s.specialization = t.required_specialization)") == 0, "every emergency type can be treated by at least one hospital")
ok(one("SELECT COUNT(*) FROM hospitals h JOIN nodes n ON n.id = h.location_id WHERE n.node_type != 'HOSPITAL'") == 0
   and one("SELECT COUNT(*) FROM nodes WHERE node_type = 'HOSPITAL'") == counts["hospitals"], "hospital nodes and hospitals match one-to-one")
ok(one("SELECT COUNT(*) FROM (SELECT location_id FROM hospitals GROUP BY location_id HAVING COUNT(*) > 1)") == 0, "at most one hospital per node")
ok(one("SELECT COUNT(*) FROM ambulances WHERE status = 'FREE'") >= 3, "at least 3 free ambulances in the default state")

print("--- city.json matches the database ---")
city = json.load(open(f"{D}/city.json"))
ok(len(city["nodes"]) == counts["nodes"] and len(city["roads"]) == counts["roads"]
   and len(city["hospitals"]) == counts["hospitals"] and len(city["scenarios"]) == counts["scenarios"], "city.json counts equal database counts")

# ---------- reference copy of the Dispatcher's matching rules ----------
_cache = {}
def dij(src):
    if src in _cache: return _cache[src]
    dist, pq = {src: 0}, [(0, src)]
    while pq:
        d, n = heapq.heappop(pq)
        if d > dist[n]: continue
        for m, w in adj.get(n, []):
            if m not in dist or d + w < dist[m]: dist[m] = d + w; heapq.heappush(pq, (d + w, m))
    _cache[src] = dist
    return dist

def build_state(sid):
    hosp = {r[0]: dict(loc=r[1], avail=r[2], specs=set()) for r in q("SELECT id, location_id, available_beds FROM hospitals ORDER BY id")}
    for h, s in q("SELECT hospital_id, specialization FROM hospital_specializations"): hosp[h]["specs"].add(s)
    ambs = [dict(id=r[0], loc=r[1], status=r[2]) for r in q("SELECT id, location_id, status FROM ambulances ORDER BY id")]
    types = {r[0]: (r[1], r[2]) for r in q("SELECT type, severity, required_specialization FROM emergency_types")}
    if sid is None:
        rows = q("SELECT id, location_id, emergency_type FROM emergencies ORDER BY reported_at, id")
    else:
        for h, b in q("SELECT hospital_id, available_beds FROM scenario_hospital_beds WHERE scenario_id = ?", sid): hosp[h]["avail"] = b
        ov = {r[0]: (r[1], r[2]) for r in q("SELECT ambulance_id, location_id, status FROM scenario_ambulances WHERE scenario_id = ?", sid)}
        for a in ambs:
            if a["id"] in ov: a["loc"], a["status"] = ov[a["id"]]
        rows = q("SELECT id, location_id, emergency_type FROM scenario_emergencies WHERE scenario_id = ? ORDER BY report_order", sid)
    queue = [dict(id=r[0], loc=r[1], type=r[2], sev=types[r[2]][0], spec=types[r[2]][1], order=i) for i, r in enumerate(rows)]
    return hosp, ambs, queue

def best_hospital(hosp, e):  # Dispatcher::findBestFeasibleHospital
    best, best_score = None, float("-inf")
    for hid, h in hosp.items():
        if e["spec"] in h["specs"] and h["avail"] > 0 and h["loc"] in dij(e["loc"]):
            score = -dij(e["loc"])[h["loc"]] + 0.5 * h["avail"]
            if score > best_score: best, best_score = hid, score
    return best

def first_match(hosp, ambs, queue):  # Dispatcher::tryMatch: best (free ambulance, emergency) pair
    best, trace, feasible = None, [], []
    for e in sorted(queue, key=lambda e: (-e["sev"], e["order"]))[:5]:
        h = best_hospital(hosp, e)
        if h is None: trace.append((e["type"], "skipped: no feasible hospital")); continue
        feasible.append((e, h))
    for amb in ambs:
        if amb["status"] != "FREE": continue
        for e, h in feasible:
            score = 10 * e["sev"] - dij(amb["loc"])[e["loc"]]
            trace.append((e["type"], score))
            if best is None or score > best[0]: best = (score, e["type"], h)
    return best, trace

print("--- Scenarios replayed through the Dispatcher rules ---")
expected = {None: "match", 1: "match", 2: "Cardiac Arrest is skipped", 3: "Breathing Difficulty", 4: "Cardiac Arrest"}
names = {None: "Default state"} | {r[0]: r[1] for r in q("SELECT id, name FROM scenarios")}
for sid, want in expected.items():
    hosp, ambs, queue = build_state(sid)
    best, trace = first_match(hosp, ambs, queue)
    print(f"      {names[sid]}: {trace} -> winner: {best[1] if best else None}")
    if want == "match": ok(best is not None, f"{names[sid]}: the Dispatcher finds a match")
    elif want == "Cardiac Arrest is skipped":
        ok(any(t == "Cardiac Arrest" and not isinstance(s, (int, float)) for t, s in trace) and best and best[1] != "Cardiac Arrest",
           f"{names[sid]}: cardiac case skipped, another case served")
    else: ok(best is not None and best[1] == want, f"{names[sid]}: winner is {want}")

print()
if fails:
    print(f"{len(fails)} CHECK(S) FAILED"); sys.exit(1)
print("ALL CHECKS PASSED")
