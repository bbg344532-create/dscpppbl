// Initial state for the MedRoute dashboard. Mirrors database/seed_data.sql.
// Graph topology/weights live in engine/graph.js (single source of truth,
// reused by Dijkstra and the map rendering alike).

export const initialHospitals = [
  { id: 1, name: 'City General Hospital', locationId: 8, availableBeds: 12, totalBeds: 20, specializations: ['GENERAL', 'TRAUMA'] },
  { id: 2, name: 'Metro Cardiac Center', locationId: 10, availableBeds: 6, totalBeds: 10, specializations: ['CARDIAC'] },
  { id: 3, name: "St. Mary's Children's Hospital", locationId: 4, availableBeds: 8, totalBeds: 12, specializations: ['PEDIATRIC', 'GENERAL'] },
  { id: 4, name: 'Riverside Trauma Center', locationId: 6, availableBeds: 5, totalBeds: 8, specializations: ['TRAUMA', 'CARDIAC'] },
  { id: 5, name: 'Northside Community Hospital', locationId: 2, availableBeds: 15, totalBeds: 18, specializations: ['GENERAL'] },
];

export const initialAmbulances = [
  { id: 101, locationId: 1, status: 'FREE' },
  { id: 102, locationId: 9, status: 'FREE' },
  { id: 103, locationId: 5, status: 'EN_ROUTE' },
  { id: 104, locationId: 7, status: 'OCCUPIED' },
  { id: 105, locationId: 3, status: 'FREE' },
];

// `order` = tie-breaker for equal severity (lower = reported earlier),
// same role Emergency::timestamp plays in the C++ PriorityQueue.
export const initialEmergencies = [
  { id: 1, locationId: 7, type: 'Cardiac Arrest', severity: 9, requiredSpecialization: 'CARDIAC', reportedAt: '14:01:12', order: 1 },
  { id: 2, locationId: 10, type: 'Fracture', severity: 4, requiredSpecialization: 'GENERAL', reportedAt: '14:02:40', order: 2 },
  { id: 3, locationId: 6, type: 'Severe Bleeding', severity: 7, requiredSpecialization: 'TRAUMA', reportedAt: '14:03:55', order: 3 },
  { id: 4, locationId: 2, type: 'Breathing Difficulty', severity: 7, requiredSpecialization: 'GENERAL', reportedAt: '14:05:02', order: 4 },
  { id: 5, locationId: 5, type: 'Minor Injury', severity: 2, requiredSpecialization: 'GENERAL', reportedAt: '14:06:18', order: 5 },
];

export const initialAssignedHospitalByAmbulance = {};

export const initialDispatchLog = [];

// Triage-style severity banding — same red/orange/yellow/green convention
// real ESI triage tags use, so operators can scan priority at a glance.
export function severityBand(severity) {
  if (severity >= 8) return { label: 'Critical', color: 'var(--sev-critical)' };
  if (severity >= 6) return { label: 'Urgent', color: 'var(--sev-urgent)' };
  if (severity >= 3) return { label: 'Moderate', color: 'var(--sev-moderate)' };
  return { label: 'Minor', color: 'var(--sev-minor)' };
}

export function statusColor(status) {
  if (status === 'FREE') return 'var(--status-free)';
  if (status === 'EN_ROUTE') return 'var(--status-enroute)';
  return 'var(--status-occupied)';
}
