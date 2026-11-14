#!/usr/bin/env python3
"""Cross-check for the MedRoute service (Phase-III, Session 4).

Runs the compiled service on the default state and every demo scenario in city.json, and compares
every answer with an independent Python model of the same rules. The service is always fed its
OWN previous output as the next input, so this also tests saving and restoring state.

Usage:  python cross_check.py <path to medroute_service> [path to city.json]
"""
import copy, heapq, json, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
EXE = sys.argv[1] if len(sys.argv) > 1 else "medroute_service"
CITY_PATH = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, "..", "..", "database", "city.json")
CITY = json.load(open(CITY_PATH))
fails, checks = [], 0

def ok(cond, msg):
    global checks
    checks += 1
    if not cond:
        fails.append(msg); print("FAIL  " + msg)

# ---------------------------------------------------------------- state and input text
def initial_state(sid):
    hospitals = {h["id"]: dict(node=h["node"], total=h["total_beds"], avail=h["available_beds"], specs=list(h["specializations"])) for h in CITY["hospitals"]}
    ambulances = {a["id"]: dict(node=a["node"], status=a["status"]) for a in CITY["ambulances"]}
    if sid is None:
        emergencies = [(i + 1, e["node"], i + 1, e["type"]) for i, e in enumerate(CITY["emergencies"])]
    else:
        sc = next(s for s in CITY["scenarios"] if s["id"] == sid)
        for hb in sc["hospital_beds"]: hospitals[hb["hospital_id"]]["avail"] = hb["available_beds"]
        for a in sc["ambulances"]: ambulances[a["id"]] = dict(node=a["node"], status=a["status"])
        emergencies = [(i + 1, e["node"], i + 1, e["type"]) for i, e in enumerate(sc["emergencies"])]
    return dict(types=[(t["type"], t["severity"], t["required_specialization"]) for t in CITY["emergency_types"]],
                roads=[[r["from_node"], r["to_node"], r["weight"]] for r in CITY["roads"]],
                hospitals=hospitals, ambulances=ambulances, emergencies=emergencies, trips=[])

def to_text(st, action):
    L = ["TYPE %d %s %s" % (s, p, n) for n, s, p in st["types"]]
    L += ["ROAD %d %d %s" % (a, b, w) for a, b, w in st["roads"]]
    L += ["HOSPITAL %d %d %d %d %s" % (i, h["node"], h["total"], h["avail"], ",".join(h["specs"])) for i, h in st["hospitals"].items()]
    L += ["AMBULANCE %d %d %s" % (i, a["node"], a["status"]) for i, a in st["ambulances"].items()]
    L += ["EMERGENCY %d %d %d %s" % (i, n, o, t) for i, n, o, t in st["emergencies"]]
    L += ["TRIP %d %d %d %d" % t for t in st["trips"]]
    return "\n".join(L + ["ACTION " + action]) + "\n"

def call(text):
    r = subprocess.run([EXE], input=text, capture_output=True, text=True)
    return json.loads(r.stdout)

def from_output(st, out, new_emergency=None):
    """Next input state, built only from what the service reported."""
    nxt = copy.deepcopy(st)
    for a in out["ambulances"]: nxt["ambulances"][a["id"]] = dict(node=a["node"], status=a["status"])
    for h in out["hospitals"]: nxt["hospitals"][h["id"]]["avail"] = h["availableBeds"]
    known = list(nxt["emergencies"]) + ([new_emergency] if new_emergency else [])
    nxt["emergencies"] = [e for e in known if e[0] in out["pending"]]
    nxt["trips"] = [(t["ambulanceId"], t["emergencyId"], t["patientNode"], t["hospitalId"]) for t in out["trips"]]
    for rc in out["roadChanges"]:
        for r in nxt["roads"]:
            if {r[0], r[1]} == {rc["from"], rc["to"]}: r[2] = rc["cost"]
    return nxt

# ---------------------------------------------------------------- independent model of the rules
def dijkstra(roads, src):
    adj = {}
    for a, b, w in roads: adj.setdefault(a, []).append((b, w)); adj.setdefault(b, []).append((a, w))
    dist, pq = {src: 0}, [(0, src)]
    while pq:
        d, n = heapq.heappop(pq)
        if d > dist[n]: continue
        for m, w in adj.get(n, []):
            if m not in dist or d + w < dist[m]: dist[m] = d + w; heapq.heappush(pq, (d + w, m))
    return dist

def model_cycle(st):
    types = {n: (s, p) for n, s, p in st["types"]}
    out = []
    while True:
        queue = sorted(st["emergencies"], key=lambda e: (-types[e[3]][0], e[2]))[:5]
        feasible = []
        for e in queue:
            dist_e = dijkstra(st["roads"], e[1]); best = None
            for hid, h in st["hospitals"].items():
                if types[e[3]][1] in h["specs"] and h["avail"] > 0 and h["node"] in dist_e:
                    score = -dist_e[h["node"]] + 0.5 * h["avail"]
                    if best is None or score > best[0]: best = (score, hid, dist_e[h["node"]])
            if best: feasible.append((e, best[1], best[2]))
        best = None
        for aid, a in st["ambulances"].items():
            if a["status"] != "FREE": continue
            d = dijkstra(st["roads"], a["node"])
            for e, hid, hdist in feasible:
                if e[1] not in d: continue
                score = 10 * types[e[3]][0] - d[e[1]]
                if best is None or score > best[0]: best = (score, aid, e, hid, d[e[1]], hdist)
        if best is None: return out
        _, aid, e, hid, pickup, hdist = best
        st["emergencies"].remove(e); st["hospitals"][hid]["avail"] -= 1; st["ambulances"][aid]["status"] = "EN_ROUTE"
        st["trips"].append((aid, e[0], e[1], hid)); out.append((e[0], aid, hid, pickup, hdist))

def model_complete(st, aid):
    trip = next(t for t in st["trips"] if t[0] == aid)
    st["trips"].remove(trip)
    st["ambulances"][aid] = dict(node=st["hospitals"][trip[3]]["node"], status="FREE")
    return model_cycle(st)

def trip_cost(st, trip):
    a = st["ambulances"][trip[0]]
    return dijkstra(st["roads"], a["node"])[trip[2]] + dijkstra(st["roads"], trip[2])[st["hospitals"][trip[3]]["node"]]

# ---------------------------------------------------------------- comparisons
def compare(label, out, expected, st):
    if not out.get("ok"): ok(False, "%s: service reported an error: %s" % (label, out.get("error"))); return
    got = [(d["emergencyId"], d["ambulanceId"], d["hospitalId"], d["pickupDistance"], d["hospitalDistance"]) for d in out["dispatches"]]
    ok(got == expected, "%s: dispatches %s should equal %s" % (label, got, expected))
    types = {n: (s, p) for n, s, p in st["types"]}
    ok({a["id"]: (a["node"], a["status"]) for a in out["ambulances"]} == {i: (a["node"], a["status"]) for i, a in st["ambulances"].items()}, label + ": ambulances")
    ok({h["id"]: h["availableBeds"] for h in out["hospitals"]} == {i: h["avail"] for i, h in st["hospitals"].items()}, label + ": hospital beds")
    ok(out["pending"] == [e[0] for e in sorted(st["emergencies"], key=lambda e: (-types[e[3]][0], e[2]))], label + ": waiting emergencies in priority order")
    ok(sorted((t["ambulanceId"], t["emergencyId"], t["hospitalId"]) for t in out["trips"]) == sorted((t[0], t[1], t[3]) for t in st["trips"]), label + ": trips")

def expect_error(label, text, error_type):
    out = call(text)
    ok(out.get("ok") is False and out["error"]["type"] == error_type, "%s: expected %s, got %s" % (label, error_type, out.get("error")))

# ---------------------------------------------------------------- the scripted sessions
def session(label, sid):
    ref = initial_state(sid); svc = copy.deepcopy(ref)

    out = call(to_text(svc, "run")); compare(label + " run", out, model_cycle(ref), ref)
    ok(call(to_text(svc, "run")) == out, label + " run: same input gives identical output")
    first = out["dispatches"][0]["ambulanceId"] if out["dispatches"] else None
    svc = from_output(svc, out)

    if first is not None:
        out = call(to_text(svc, "complete %d" % first)); compare(label + " complete", out, model_complete(ref, first), ref)
        ok(len(out["completed"]) == 1 and out["completed"][0]["ambulanceId"] == first, label + " complete: trip closed")
        svc = from_output(svc, out)

    new = (100, 11, max([e[2] for e in svc["emergencies"]] + [0]) + 1, "Burns")
    out = call(to_text(svc, "report 100 11 Burns"))
    ref["emergencies"].append(new); compare(label + " report", out, model_cycle(ref), ref)
    svc = from_output(svc, out, new)

    if out["trips"]:
        route = out["trips"][0]["route"]; a, b = route[0], route[1]
        old = {t: trip_cost(ref, t) for t in ref["trips"]}
        w = next(r[2] for r in ref["roads"] if {r[0], r[1]} == {a, b})
        for r in ref["roads"]:
            if {r[0], r[1]} == {a, b}: r[2] = w * 6
        expected = sorted((t[0], trip_cost(ref, t)) for t in ref["trips"] if trip_cost(ref, t) != old[t])
        out = call(to_text(svc, "traffic %d %d %s" % (a, b, w * 6)))
        got = sorted((r["ambulanceId"], r["newCost"]) for r in out["reroutes"] if r["newCost"] != r["oldCost"])
        ok(out["ok"] and got == expected, "%s traffic on road %d-%d: re-planned costs %s should equal %s" % (label, a, b, got, expected))
        svc = from_output(svc, out)

    out = call(to_text(svc, "discharge 1"))
    h = ref["hospitals"][1]
    if h["avail"] < h["total"]: h["avail"] += 1; expected = model_cycle(ref)
    else: expected = []
    compare(label + " discharge", out, expected, ref)

print("Service cross-check against the Python model")
session("Default state", None)
for s in CITY["scenarios"]: session("Scenario %d (%s)" % (s["id"], s["name"]), s["id"])

base = to_text(initial_state(None), "state")
amb_free = next(i for i, a in initial_state(None)["ambulances"].items() if a["status"] == "FREE")
expect_error("report at an unknown node", to_text(initial_state(None), "report 100 999 Burns"), "UnknownLocationError")
expect_error("complete an ambulance that is not en route", to_text(initial_state(None), "complete %d" % amb_free), "InvalidArgumentError")
expect_error("traffic on a road that does not exist", to_text(initial_state(None), "traffic 1 999 5"), "UnknownRoadError")
expect_error("traffic with a zero cost", to_text(initial_state(None), "traffic 1 2 0"), "InvalidArgumentError")
expect_error("discharge an unknown hospital", to_text(initial_state(None), "discharge 999"), "UnknownIdError")
expect_error("duplicate emergency id", to_text(initial_state(None), "report 1 11 Burns"), "DuplicateIdError")
expect_error("unknown emergency type", to_text(initial_state(None), "report 100 11 Nonsense"), "InputError")
expect_error("malformed line", base + "ROAD x y z\n", "InputError")
expect_error("no action", base.replace("ACTION state\n", ""), "InputError")
expect_error("two actions", base + "ACTION run\n", "InputError")
expect_error("duplicate ambulance in the input", base.replace("ACTION state", "AMBULANCE 101 1 FREE\nACTION state"), "DuplicateIdError")

print("\n%d checks, %d failed" % (checks, len(fails)))
print("ALL CROSS-CHECKS PASSED" if not fails else "CROSS-CHECK FAILED")
sys.exit(1 if fails else 0)
