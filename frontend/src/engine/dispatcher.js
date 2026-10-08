import { shortestPath } from './dijkstra';

// Direct port of core-engine/src/Dispatcher.cpp's matching logic, so this
// dashboard demonstrates the REAL algorithm (feasibility-skip + combined
// severity/distance scoring), not a simplified stand-in. Same constants:
const TOP_K = 5;
const SEVERITY_WEIGHT = 10;
const DISTANCE_WEIGHT = 1;
const HOSPITAL_LOAD_WEIGHT = 0.5;

function getTopK(emergencies, k) {
  return [...emergencies]
    .sort((a, b) => (b.severity - a.severity) || (a.order - b.order))
    .slice(0, k);
}

// Mirrors Dispatcher::findBestFeasibleHospital — filters by specialization
// AND bed availability, then scores remaining ones by distance + load.
function findBestFeasibleHospital(emergency, fromLocationId, hospitals) {
  let best = null;
  let bestScore = -Infinity;

  for (const h of hospitals) {
    const suitable = h.specializations.includes(emergency.requiredSpecialization) && h.availableBeds > 0;
    if (!suitable) continue;

    const { reachable, distance } = shortestPath(fromLocationId, h.locationId);
    if (!reachable) continue;

    const score = -DISTANCE_WEIGHT * distance + HOSPITAL_LOAD_WEIGHT * h.availableBeds;
    if (score > bestScore) {
      bestScore = score;
      best = { hospital: h, distance };
    }
  }
  return best;
}

// Mirrors Dispatcher::tryMatch — peeks top-K by severity, skips any with no
// feasible hospital (they stay queued, not removed), scores the rest by
// severity*10 - distance, and picks the best. Pushes human-readable
// reasoning into `trace` so the UI can show exactly why each decision was made.
function tryMatch(ambulance, emergencies, hospitals, trace) {
  const candidates = getTopK(emergencies, TOP_K);
  if (candidates.length === 0) {
    trace.push(`AMB-${ambulance.id}: no emergencies waiting.`);
    return null;
  }

  let best = null;
  let bestScore = -Infinity;

  for (const e of candidates) {
    const feasible = findBestFeasibleHospital(e, e.locationId, hospitals);
    if (!feasible) {
      trace.push(`  skip EMG-${e.id} (${e.type}, sev ${e.severity}) — no hospital currently has ${e.requiredSpecialization} capacity`);
      continue;
    }
    const { reachable, distance } = shortestPath(ambulance.locationId, e.locationId);
    if (!reachable) {
      trace.push(`  skip EMG-${e.id} — unreachable from AMB-${ambulance.id}`);
      continue;
    }
    const score = SEVERITY_WEIGHT * e.severity - DISTANCE_WEIGHT * distance;
    trace.push(`  EMG-${e.id} (sev ${e.severity}, dist ${distance}) → score ${score.toFixed(1)}`);
    if (score > bestScore) {
      bestScore = score;
      best = { emergency: e, hospital: feasible.hospital, pickupDistance: distance, hospitalDistance: feasible.distance };
    }
  }

  if (!best) {
    trace.push(`AMB-${ambulance.id}: nothing feasible among top ${Math.min(TOP_K, candidates.length)} — staying free.`);
    return null;
  }

  trace.push(`→ AMB-${ambulance.id} dispatched to EMG-${best.emergency.id} → HOSP-${best.hospital.id} (score ${bestScore.toFixed(1)})`);
  return best;
}

// Mirrors the matching loop in Dispatcher::reportEmergency / completeDispatch:
// keep matching free ambulances to feasible emergencies until nothing more
// can be matched. Returns a fresh copy of state plus a trace of the reasoning
// and any new dispatch-log entries created this cycle.
export function runDispatchCycle(state) {
  let { ambulances, hospitals, emergencies, assignedHospitalByAmbulance } = state;
  const trace = [];
  const newLogEntries = [];
  let matchedAny = true;

  while (matchedAny) {
    matchedAny = false;
    if (emergencies.length === 0) break;

    for (const amb of ambulances) {
      if (amb.status !== 'FREE') continue;
      const result = tryMatch(amb, emergencies, hospitals, trace);
      if (!result) continue;

      const pickupPath = shortestPath(amb.locationId, result.emergency.locationId).path;
      const hospitalPath = shortestPath(result.emergency.locationId, result.hospital.locationId).path;

      emergencies = emergencies.filter((e) => e.id !== result.emergency.id);
      hospitals = hospitals.map((h) =>
        h.id === result.hospital.id ? { ...h, availableBeds: h.availableBeds - 1 } : h
      );
      ambulances = ambulances.map((a) =>
        a.id === amb.id ? { ...a, status: 'EN_ROUTE' } : a
      );
      assignedHospitalByAmbulance = { ...assignedHospitalByAmbulance, [amb.id]: result.hospital.locationId };

      const time = new Date().toTimeString().slice(0, 8);
      newLogEntries.push({
        time,
        emergencyId: result.emergency.id,
        ambulanceId: amb.id,
        hospitalId: result.hospital.id,
        pickupDist: result.pickupDistance,
        hospitalDist: result.hospitalDistance,
        path: [...pickupPath, ...hospitalPath.slice(1)],
      });

      matchedAny = true;
      break; // restart the scan since state changed
    }
  }

  return { ambulances, hospitals, emergencies, assignedHospitalByAmbulance, trace, newLogEntries };
}

// Mirrors Dispatcher::completeDispatch — frees the ambulance, moves it to
// where it was headed, then immediately re-runs the matching cycle so any
// emergency that was waiting on exactly this ambulance gets served.
export function completeDispatch(ambulanceId, state) {
  const hospitalLocation = state.assignedHospitalByAmbulance[ambulanceId];

  const ambulances = state.ambulances.map((a) =>
    a.id === ambulanceId
      ? { ...a, status: 'FREE', locationId: hospitalLocation ?? a.locationId }
      : a
  );
  const assignedHospitalByAmbulance = { ...state.assignedHospitalByAmbulance };
  delete assignedHospitalByAmbulance[ambulanceId];

  const trace = [`AMB-${ambulanceId} completed its trip — now FREE at loc ${hospitalLocation ?? '?'}.`];

  const afterCycle = runDispatchCycle({ ...state, ambulances, assignedHospitalByAmbulance });
  return { ...afterCycle, trace: [...trace, ...afterCycle.trace] };
}
