// MedRoute service: the core engine as a command-line program.
//
// Reads a saved state plus ONE action (from the file named on the command line, or from standard
// input), applies the action with the real Dispatcher, and prints the result as JSON.
//
// It is stateless: every run rebuilds the engine from the state it is given, so the backend and
// the database own the data and this program only makes decisions. An action either succeeds
// completely or reports an error and changes nothing (nothing is saved here).
//
// INPUT FORMAT (one item per line; blank lines and lines starting with # are ignored)
//   TYPE <severity> <SPECIALIZATION> <emergency type name...>
//   ROAD <fromNode> <toNode> <cost>                           (two-way road)
//   HOSPITAL <id> <node> <totalBeds> <availableBeds> <SPEC[,SPEC...]>
//   AMBULANCE <id> <node> <FREE|EN_ROUTE|OCCUPIED>
//   EMERGENCY <id> <node> <reportOrder> <emergency type name...>        (a waiting emergency)
//   TRIP <ambulanceId> <emergencyId> <patientNode> <hospitalId>         (an ambulance already en route)
//   ACTION state | run | report <id> <node> <type name...> | complete <ambulanceId>
//          | discharge <hospitalId> | traffic <fromNode> <toNode> <newCost>
// SPECIALIZATION is GENERAL, TRAUMA, CARDIAC or PEDIATRIC.
//
// OUTPUT: one JSON object. {"ok":false,"error":{...}} on failure, otherwise "ok":true plus the
// engine's log and reasoning, the new dispatches, completed/re-routed trips, and the full new state.
#include <algorithm>
#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>
#include "Dispatcher.h"
#include "Exceptions.h"

namespace {

// ---------------------------------------------------------------- parsing
struct TypeInfo { int severity; Specialization spec; };
struct RoadLine { int a, b; double cost; };
struct HospitalLine { int id, node, total, available; std::vector<Specialization> specs; };
struct AmbulanceLine { int id, node; AmbulanceStatus status; };
struct EmergencyLine { int id, node; long order; std::string type; };
struct TripLine { int ambulanceId, emergencyId, patientNode, hospitalId; };

struct Input {
    std::map<std::string, TypeInfo> types;
    std::vector<RoadLine> roads;
    std::vector<HospitalLine> hospitals;
    std::vector<AmbulanceLine> ambulances;
    std::vector<EmergencyLine> emergencies;
    std::vector<TripLine> trips;
    std::string actionKind;
    std::string actionText;   // everything after the action word
    int actionLine;
    Input() : actionLine(0) {}
};

class Tokens {
    std::istringstream in;
    int lineNo;
    std::string where() const { std::ostringstream os; os << "line " << lineNo << ": "; return os.str(); }
public:
    Tokens(const std::string& text, int line) : in(text), lineNo(line) {}
    std::string word(const char* what) {
        std::string w;
        if (!(in >> w)) throw InputError(where() + "expected " + what);
        return w;
    }
    int integer(const char* what) {
        std::string w = word(what);
        std::istringstream conv(w);
        int v = 0; char extra = 0;
        if (!(conv >> v) || (conv >> extra)) throw InputError(where() + "expected a whole number for " + what + ", got '" + w + "'");
        return v;
    }
    double number(const char* what) {
        std::string w = word(what);
        std::istringstream conv(w);
        double v = 0; char extra = 0;
        if (!(conv >> v) || (conv >> extra)) throw InputError(where() + "expected a number for " + what + ", got '" + w + "'");
        return v;
    }
    std::string rest(const char* what) {
        std::string r;
        std::getline(in, r);
        std::size_t a = r.find_first_not_of(" \t\r"), b = r.find_last_not_of(" \t\r");
        if (a == std::string::npos) throw InputError(where() + "expected " + what);
        return r.substr(a, b - a + 1);
    }
    std::string fail(const std::string& message) const { return where() + message; }
};

Specialization parseSpec(const std::string& text, const Tokens& t) {
    if (text == "GENERAL") return Specialization::GENERAL;
    if (text == "TRAUMA") return Specialization::TRAUMA;
    if (text == "CARDIAC") return Specialization::CARDIAC;
    if (text == "PEDIATRIC") return Specialization::PEDIATRIC;
    throw InputError(t.fail("unknown specialization '" + text + "'"));
}

AmbulanceStatus parseStatus(const std::string& text, const Tokens& t) {
    if (text == "FREE") return AmbulanceStatus::FREE;
    if (text == "EN_ROUTE") return AmbulanceStatus::EN_ROUTE;
    if (text == "OCCUPIED") return AmbulanceStatus::OCCUPIED;
    throw InputError(t.fail("unknown ambulance status '" + text + "'"));
}

Input parseInput(const std::string& text) {
    Input in;
    std::istringstream lines(text);
    std::string line;
    int lineNo = 0;
    while (std::getline(lines, line)) {
        ++lineNo;
        std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        Tokens t(line, lineNo);
        std::string key = t.word("a keyword");
        if (key == "TYPE") {
            TypeInfo info; info.severity = t.integer("severity");
            info.spec = parseSpec(t.word("specialization"), t);
            in.types[t.rest("emergency type name")] = info;
        } else if (key == "ROAD") {
            RoadLine r; r.a = t.integer("from node"); r.b = t.integer("to node"); r.cost = t.number("cost");
            if (r.cost <= 0) throw InputError(t.fail("road cost must be positive"));
            in.roads.push_back(r);
        } else if (key == "HOSPITAL") {
            HospitalLine h; h.id = t.integer("hospital id"); h.node = t.integer("node");
            h.total = t.integer("total beds"); h.available = t.integer("available beds");
            if (h.total < 0 || h.available < 0 || h.available > h.total) throw InputError(t.fail("available beds must be between 0 and total beds"));
            std::string specs = t.word("specializations");
            std::istringstream parts(specs); std::string part;
            while (std::getline(parts, part, ',')) h.specs.push_back(parseSpec(part, t));
            in.hospitals.push_back(h);
        } else if (key == "AMBULANCE") {
            AmbulanceLine a; a.id = t.integer("ambulance id"); a.node = t.integer("node");
            a.status = parseStatus(t.word("status"), t);
            in.ambulances.push_back(a);
        } else if (key == "EMERGENCY") {
            EmergencyLine e; e.id = t.integer("emergency id"); e.node = t.integer("node");
            e.order = t.integer("report order"); e.type = t.rest("emergency type name");
            in.emergencies.push_back(e);
        } else if (key == "TRIP") {
            TripLine tr; tr.ambulanceId = t.integer("ambulance id"); tr.emergencyId = t.integer("emergency id");
            tr.patientNode = t.integer("patient node"); tr.hospitalId = t.integer("hospital id");
            in.trips.push_back(tr);
        } else if (key == "ACTION") {
            if (!in.actionKind.empty()) throw InputError(t.fail("only one ACTION is allowed"));
            in.actionKind = t.word("an action");
            in.actionLine = lineNo;
            std::string r; std::getline(std::istringstream(line.substr(line.find(in.actionKind) + in.actionKind.size())), r);
            std::size_t a = r.find_first_not_of(" \t\r"), b = r.find_last_not_of(" \t\r");
            in.actionText = (a == std::string::npos) ? "" : r.substr(a, b - a + 1);
        } else {
            throw InputError(t.fail("unknown keyword '" + key + "'"));
        }
    }
    if (in.actionKind.empty()) throw InputError("no ACTION line found");
    return in;
}

// ---------------------------------------------------------------- output helpers
std::string jstr(const std::string& s) {
    std::string out = "\"";
    for (std::size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c < 0x20) { char buf[8]; std::snprintf(buf, sizeof(buf), "\\u%04x", c); out += buf; }
        else out += s[i];
    }
    return out + "\"";
}
std::string jnum(double v) { std::ostringstream os; os.precision(12); os << v; return os.str(); }
std::string jroute(const std::vector<int>& r) {
    std::string out = "[";
    for (std::size_t i = 0; i < r.size(); ++i) { if (i) out += ","; out += jnum(r[i]); }
    return out + "]";
}
std::string jlines(const std::vector<std::string>& v) {
    std::string out = "[";
    for (std::size_t i = 0; i < v.size(); ++i) { if (i) out += ","; out += jstr(v[i]); }
    return out + "]";
}

std::string errorType(const std::exception& e) {
    if (dynamic_cast<const UnknownLocationError*>(&e)) return "UnknownLocationError";
    if (dynamic_cast<const DuplicateIdError*>(&e)) return "DuplicateIdError";
    if (dynamic_cast<const UnknownIdError*>(&e)) return "UnknownIdError";
    if (dynamic_cast<const UnknownRoadError*>(&e)) return "UnknownRoadError";
    if (dynamic_cast<const InvalidArgumentError*>(&e)) return "InvalidArgumentError";
    if (dynamic_cast<const InputError*>(&e)) return "InputError";
    if (dynamic_cast<const EmptyQueueError*>(&e)) return "EmptyQueueError";
    if (dynamic_cast<const MedRouteError*>(&e)) return "MedRouteError";
    return "InternalError";
}

// Captures what the engine prints (its log lines) while an action runs.
class Capture {
    std::ostringstream buf; std::streambuf* old; bool active;
public:
    Capture() : old(std::cout.rdbuf(buf.rdbuf())), active(true) {}
    ~Capture() { if (active) std::cout.rdbuf(old); }
    std::string finish() { if (active) { std::cout.rdbuf(old); active = false; } return buf.str(); }
};

std::string tripJson(const ActiveTrip& t) {
    return "{\"ambulanceId\":" + jnum(t.ambulanceId) + ",\"emergencyId\":" + jnum(t.emergencyId) +
           ",\"patientNode\":" + jnum(t.patientNode) +
           ",\"hospitalId\":" + jnum(t.hospitalId) + ",\"pickupDistance\":" + jnum(t.pickupDistance) +
           ",\"hospitalDistance\":" + jnum(t.hospitalDistance) + ",\"cost\":" + jnum(t.cost) +
           ",\"route\":" + jroute(t.route) + "}";
}

const ActiveTrip* findTrip(const std::vector<ActiveTrip>& trips, const ActiveTrip& like) {
    for (std::size_t i = 0; i < trips.size(); ++i)
        if (trips[i].ambulanceId == like.ambulanceId && trips[i].emergencyId == like.emergencyId) return &trips[i];
    return nullptr;
}

// ---------------------------------------------------------------- the work
std::string process(const std::string& text) {
    Input in = parseInput(text);

    Graph graph;
    for (std::size_t i = 0; i < in.roads.size(); ++i) graph.addEdge(in.roads[i].a, in.roads[i].b, in.roads[i].cost);
    Dispatcher dispatcher(graph);
    for (std::map<std::string, TypeInfo>::const_iterator it = in.types.begin(); it != in.types.end(); ++it)
        dispatcher.getSeverityTable().setSeverity(it->first, it->second.severity);

    std::deque<Hospital> hospitals;
    for (std::size_t i = 0; i < in.hospitals.size(); ++i) {
        const HospitalLine& h = in.hospitals[i];
        hospitals.push_back(Hospital(h.id, h.node, h.total));
        for (int k = 0; k < h.total - h.available; ++k) hospitals.back().admitPatient();   // beds already in use
        for (std::size_t s = 0; s < h.specs.size(); ++s) hospitals.back().addSpecialization(h.specs[s]);
        dispatcher.addHospital(&hospitals.back());
    }
    std::deque<Ambulance> ambulances;
    for (std::size_t i = 0; i < in.ambulances.size(); ++i) {
        ambulances.push_back(Ambulance(in.ambulances[i].id, in.ambulances[i].node));
        ambulances.back().setStatus(in.ambulances[i].status);
        dispatcher.addAmbulance(&ambulances.back());
    }
    std::vector<EmergencyLine> waiting = in.emergencies;
    std::stable_sort(waiting.begin(), waiting.end(), [](const EmergencyLine& x, const EmergencyLine& y) { return x.order < y.order; });
    for (std::size_t i = 0; i < waiting.size(); ++i) {
        std::map<std::string, TypeInfo>::const_iterator type = in.types.find(waiting[i].type);
        if (type == in.types.end()) throw InputError("unknown emergency type '" + waiting[i].type + "'");
        dispatcher.restoreEmergency(waiting[i].id, waiting[i].node, waiting[i].type, type->second.spec, waiting[i].order);
    }
    for (std::size_t i = 0; i < in.trips.size(); ++i)
        dispatcher.restoreTrip(in.trips[i].ambulanceId, in.trips[i].emergencyId, in.trips[i].patientNode, in.trips[i].hospitalId);

    const std::vector<ActiveTrip> before = dispatcher.getActiveTrips();
    dispatcher.clearTrace();

    // ---- apply the one action, capturing the engine's printed log
    std::string roadChange;
    std::string logText;
    {
        Capture capture;
        const std::string& kind = in.actionKind;
        Tokens args(in.actionText, in.actionLine);
        if (kind == "state") {
            // nothing to do: just report the state
        } else if (kind == "run") {
            dispatcher.dispatchWaiting();
        } else if (kind == "report") {
            int id = args.integer("emergency id"); int node = args.integer("node");
            std::string type = args.rest("emergency type name");
            std::map<std::string, TypeInfo>::const_iterator info = in.types.find(type);
            if (info == in.types.end()) throw InputError("unknown emergency type '" + type + "'");
            dispatcher.reportEmergency(id, node, type, info->second.spec);
        } else if (kind == "complete") {
            int id = args.integer("ambulance id");
            const Ambulance* found = nullptr;
            for (std::size_t i = 0; i < ambulances.size(); ++i) if (ambulances[i].getId() == id) found = &ambulances[i];
            if (found == nullptr) throw UnknownIdError("ambulance", id);
            if (found->getStatus() != AmbulanceStatus::EN_ROUTE) {
                std::ostringstream os; os << "Ambulance " << id << " is not en route";
                throw InvalidArgumentError(os.str());
            }
            dispatcher.completeDispatch(id);
        } else if (kind == "discharge") {
            dispatcher.dischargePatient(args.integer("hospital id"));
        } else if (kind == "traffic") {
            int a = args.integer("from node"); int b = args.integer("to node"); double cost = args.number("new cost");
            dispatcher.applyTrafficEvent(a, b, cost);
            roadChange = "{\"from\":" + jnum(a) + ",\"to\":" + jnum(b) + ",\"cost\":" + jnum(cost) + "}";
        } else {
            throw InputError("unknown action '" + kind + "'");
        }
        logText = capture.finish();
    }
    std::vector<std::string> log;
    { std::istringstream ls(logText); std::string l; while (std::getline(ls, l)) if (!l.empty()) log.push_back(l); }

    // ---- compare the trips before and after to find what happened
    const std::vector<ActiveTrip>& after = dispatcher.getActiveTrips();
    std::string dispatches, completed, reroutes;
    for (std::size_t i = 0; i < after.size(); ++i) {
        const ActiveTrip* old = findTrip(before, after[i]);
        if (old == nullptr) {
            dispatches += (dispatches.empty() ? "" : ",") + tripJson(after[i]);
        } else if (old->route != after[i].route || old->cost != after[i].cost) {
            reroutes += (reroutes.empty() ? "" : ",") + std::string("{\"ambulanceId\":") + jnum(after[i].ambulanceId) +
                        ",\"emergencyId\":" + jnum(after[i].emergencyId) + ",\"oldCost\":" + jnum(old->cost) +
                        ",\"newCost\":" + jnum(after[i].cost) + ",\"oldRoute\":" + jroute(old->route) +
                        ",\"newRoute\":" + jroute(after[i].route) + "}";
        }
    }
    for (std::size_t i = 0; i < before.size(); ++i)
        if (findTrip(after, before[i]) == nullptr) completed += (completed.empty() ? "" : ",") + tripJson(before[i]);

    std::string amb, hosp, pending, trips;
    for (std::size_t i = 0; i < ambulances.size(); ++i)
        amb += (i ? "," : "") + std::string("{\"id\":") + jnum(ambulances[i].getId()) + ",\"node\":" + jnum(ambulances[i].getLocationId()) +
               ",\"status\":" + jstr(toString(ambulances[i].getStatus())) + "}";
    for (std::size_t i = 0; i < hospitals.size(); ++i)
        hosp += (i ? "," : "") + std::string("{\"id\":") + jnum(hospitals[i].getId()) + ",\"totalBeds\":" + jnum(hospitals[i].getTotalBeds()) +
                ",\"availableBeds\":" + jnum(hospitals[i].getAvailableBeds()) + "}";
    std::vector<EmergencyEntry> entries = dispatcher.getPendingEntries();
    for (std::size_t i = 0; i < entries.size(); ++i) pending += (i ? "," : "") + jnum(entries[i].emergencyId);
    for (std::size_t i = 0; i < after.size(); ++i) trips += (i ? "," : "") + tripJson(after[i]);

    return "{\"ok\":true,\"action\":" + jstr(in.actionKind) + ",\"log\":" + jlines(log) + ",\"trace\":" + jlines(dispatcher.getTrace()) +
           ",\"dispatches\":[" + dispatches + "],\"completed\":[" + completed + "],\"reroutes\":[" + reroutes +
           "],\"roadChanges\":[" + roadChange + "],\"ambulances\":[" + amb + "],\"hospitals\":[" + hosp +
           "],\"pending\":[" + pending + "],\"trips\":[" + trips + "]}";
}

}  // namespace

int main(int argc, char** argv) {
    std::string text;
    if (argc > 1) {
        std::ifstream file(argv[1]);
        if (!file) { std::cerr << "Cannot open " << argv[1] << "\n"; return 2; }
        std::ostringstream buf; buf << file.rdbuf(); text = buf.str();
    } else {
        std::ostringstream buf; buf << std::cin.rdbuf(); text = buf.str();
    }
    std::string out;
    try {
        out = process(text);
    } catch (const std::exception& e) {
        out = "{\"ok\":false,\"error\":{\"type\":" + jstr(errorType(e)) + ",\"message\":" + jstr(e.what()) + "}}";
    }
    std::cout << out << "\n";
    return 0;
}
