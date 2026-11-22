// Moving state between the database and the engine.
//   loadState        reads what the engine needs
//   toEngineText     writes it in the engine's input format
//   applyResult      writes the engine's answer back to the database
//   describeState    the full picture for the frontend
const { ApiError } = require('./errors');

function loadState(db) {
  const hospitals = db.all('SELECT id, name, location_id AS node, total_beds AS total, available_beds AS avail FROM hospitals ORDER BY id')
    .map((h) => ({ ...h, specs: db.all('SELECT specialization FROM hospital_specializations WHERE hospital_id = ? ORDER BY specialization', [h.id]).map((r) => r.specialization) }));
  return {
    types: db.all('SELECT type, severity, required_specialization AS spec FROM emergency_types ORDER BY type'),
    roads: db.all('SELECT from_node AS a, to_node AS b, weight FROM roads ORDER BY id'),
    hospitals,
    ambulances: db.all('SELECT id, location_id AS node, status FROM ambulances ORDER BY id'),
    waiting: db.all("SELECT id, location_id AS node, emergency_type AS type FROM emergencies WHERE status = 'WAITING' ORDER BY id"),
    trips: db.all('SELECT h.ambulance_id AS ambulanceId, h.emergency_id AS emergencyId, e.location_id AS patientNode, h.hospital_id AS hospitalId FROM dispatch_history h JOIN emergencies e ON e.id = h.emergency_id WHERE h.completed_at IS NULL ORDER BY h.id'),
  };
}

function toEngineText(state, action) {
  const lines = [];
  state.types.forEach((t) => lines.push(`TYPE ${t.severity} ${t.spec} ${t.type}`));
  state.roads.forEach((r) => lines.push(`ROAD ${r.a} ${r.b} ${r.weight}`));
  state.hospitals.forEach((h) => lines.push(`HOSPITAL ${h.id} ${h.node} ${h.total} ${h.avail} ${h.specs.join(',')}`));
  state.ambulances.forEach((a) => lines.push(`AMBULANCE ${a.id} ${a.node} ${a.status}`));
  state.waiting.forEach((e) => lines.push(`EMERGENCY ${e.id} ${e.node} ${e.id} ${e.type}`)); // the ID rises with every report, so it doubles as the report order
  state.trips.forEach((t) => lines.push(`TRIP ${t.ambulanceId} ${t.emergencyId} ${t.patientNode} ${t.hospitalId}`));
  lines.push(`ACTION ${action}`);
  return lines.join('\n') + '\n';
}

// Writes the engine's answer to the database. Call inside db.transaction().
function applyResult(db, out, newEmergency) {
  if (newEmergency) {
    db.run("INSERT INTO emergencies (id, location_id, emergency_type, status) VALUES (?, ?, ?, 'WAITING')", [newEmergency.id, newEmergency.node, newEmergency.type]);
  }
  out.roadChanges.forEach((rc) => db.run('UPDATE roads SET weight = ? WHERE (from_node = ? AND to_node = ?) OR (from_node = ? AND to_node = ?)', [rc.cost, rc.from, rc.to, rc.to, rc.from]));
  out.ambulances.forEach((a) => db.run('UPDATE ambulances SET location_id = ?, status = ? WHERE id = ?', [a.node, a.status, a.id]));
  out.hospitals.forEach((h) => db.run('UPDATE hospitals SET available_beds = ? WHERE id = ?', [h.availableBeds, h.id]));
  out.completed.forEach((t) => {
    db.run('UPDATE dispatch_history SET completed_at = CURRENT_TIMESTAMP WHERE ambulance_id = ? AND emergency_id = ? AND completed_at IS NULL', [t.ambulanceId, t.emergencyId]);
    db.run("UPDATE emergencies SET status = 'COMPLETED' WHERE id = ?", [t.emergencyId]);
  });
  out.dispatches.forEach((d) => {
    db.run('INSERT INTO dispatch_history (emergency_id, ambulance_id, hospital_id, pickup_distance, hospital_distance, route, cost) VALUES (?, ?, ?, ?, ?, ?, ?)',
      [d.emergencyId, d.ambulanceId, d.hospitalId, d.pickupDistance, d.hospitalDistance, JSON.stringify(d.route), d.cost]);
    db.run("UPDATE emergencies SET status = 'DISPATCHED' WHERE id = ?", [d.emergencyId]);
  });
  out.reroutes.forEach((r) => db.run('UPDATE dispatch_history SET route = ?, cost = ? WHERE ambulance_id = ? AND emergency_id = ? AND completed_at IS NULL', [JSON.stringify(r.newRoute), r.newCost, r.ambulanceId, r.emergencyId]));
}

function describeState(db, historyLimit = 50) {
  const waiting = db.all("SELECT e.id, e.location_id AS node, e.emergency_type AS type, t.severity, t.required_specialization AS requiredSpecialization, e.reported_at AS reportedAt FROM emergencies e JOIN emergency_types t ON t.type = e.emergency_type WHERE e.status = 'WAITING'")
    .sort((a, b) => b.severity - a.severity || a.id - b.id);
  return {
    nodes: db.all('SELECT id, name, x, y, node_type AS type FROM nodes ORDER BY id'),
    roads: db.all('SELECT from_node AS fromNode, to_node AS toNode, weight AS cost, base_weight AS baseCost FROM roads ORDER BY id'),
    emergencyTypes: db.all('SELECT type, severity, required_specialization AS requiredSpecialization FROM emergency_types ORDER BY severity DESC, type'),
    hospitals: db.all('SELECT id, name, location_id AS node, total_beds AS totalBeds, available_beds AS availableBeds FROM hospitals ORDER BY id')
      .map((h) => ({ ...h, specializations: db.all('SELECT specialization FROM hospital_specializations WHERE hospital_id = ? ORDER BY specialization', [h.id]).map((r) => r.specialization) })),
    ambulances: db.all('SELECT id, location_id AS node, status FROM ambulances ORDER BY id'),
    waiting,
    trips: db.all('SELECT h.ambulance_id AS ambulanceId, h.emergency_id AS emergencyId, e.emergency_type AS type, h.hospital_id AS hospitalId, h.pickup_distance AS pickupDistance, h.hospital_distance AS hospitalDistance, h.cost, h.route, h.dispatched_at AS dispatchedAt FROM dispatch_history h JOIN emergencies e ON e.id = h.emergency_id WHERE h.completed_at IS NULL ORDER BY h.id')
      .map((t) => ({ ...t, route: JSON.parse(t.route) })),
    history: db.all('SELECT h.id, h.emergency_id AS emergencyId, e.emergency_type AS type, h.ambulance_id AS ambulanceId, h.hospital_id AS hospitalId, h.pickup_distance AS pickupDistance, h.hospital_distance AS hospitalDistance, h.cost, h.dispatched_at AS dispatchedAt, h.completed_at AS completedAt FROM dispatch_history h JOIN emergencies e ON e.id = h.emergency_id ORDER BY h.id DESC LIMIT ?', [historyLimit]),
    scenarios: db.all('SELECT id, name, description FROM scenarios ORDER BY id'),
  };
}

module.exports = { loadState, toEngineText, applyResult, describeState, ApiError };
