// Mock data for the MedRoute dashboard (Phase-II: frontend not yet wired to the
// backend/DB). Mirrors database/seed_data.sql and the demo graph in
// core-engine/src/main.cpp, so the dashboard tells the same story as the live
// C++ engine demo and the seeded database.

// Node positions for the schematic network map (core-engine/src/main.cpp graph:
// nodes 1-10, same edges). Coordinates are arbitrary layout positions, not real GPS.
export const graphNodes = {
  1: { x: 60, y: 260 },
  2: { x: 180, y: 140 },
  3: { x: 180, y: 340 },
  4: { x: 320, y: 140 },
  5: { x: 320, y: 340 },
  6: { x: 460, y: 200 },
  7: { x: 460, y: 340 },
  8: { x: 600, y: 140 },
  9: { x: 600, y: 400 },
  10: { x: 740, y: 260 },
};

export const graphEdges = [
  [1, 2], [1, 3], [2, 4], [3, 4], [3, 5],
  [4, 6], [5, 6], [5, 7], [6, 8], [7, 8],
  [7, 9], [8, 10], [9, 10],
];

export const hospitals = [
  { id: 1, name: 'City General Hospital', locationId: 8, beds: 12, totalBeds: 20, specializations: ['GENERAL', 'TRAUMA'] },
  { id: 2, name: 'Metro Cardiac Center', locationId: 10, beds: 6, totalBeds: 10, specializations: ['CARDIAC'] },
  { id: 3, name: "St. Mary's Children's Hospital", locationId: 4, beds: 8, totalBeds: 12, specializations: ['PEDIATRIC', 'GENERAL'] },
  { id: 4, name: 'Riverside Trauma Center', locationId: 6, beds: 5, totalBeds: 8, specializations: ['TRAUMA', 'CARDIAC'] },
  { id: 5, name: 'Northside Community Hospital', locationId: 2, beds: 15, totalBeds: 18, specializations: ['GENERAL'] },
];

export const ambulances = [
  { id: 101, locationId: 1, status: 'FREE' },
  { id: 102, locationId: 9, status: 'FREE' },
  { id: 103, locationId: 5, status: 'EN_ROUTE' },
  { id: 104, locationId: 7, status: 'OCCUPIED' },
  { id: 105, locationId: 3, status: 'FREE' },
];

// severity: higher = more critical (matches SeverityTable in the core engine)
export const emergencies = [
  { id: 1, locationId: 7, type: 'Cardiac Arrest', severity: 9, requiredSpecialization: 'CARDIAC', reportedAt: '14:01:12' },
  { id: 2, locationId: 10, type: 'Fracture', severity: 4, requiredSpecialization: 'GENERAL', reportedAt: '14:02:40' },
  { id: 3, locationId: 6, type: 'Severe Bleeding', severity: 7, requiredSpecialization: 'TRAUMA', reportedAt: '14:03:55' },
  { id: 4, locationId: 2, type: 'Breathing Difficulty', severity: 7, requiredSpecialization: 'GENERAL', reportedAt: '14:05:02' },
  { id: 5, locationId: 5, type: 'Minor Injury', severity: 2, requiredSpecialization: 'GENERAL', reportedAt: '14:06:18' },
];

export const dispatchLog = [
  { time: '14:02:03', emergencyId: 1, ambulanceId: 101, hospitalId: 2, pickupDist: 12, hospitalDist: 3 },
  { time: '14:03:10', emergencyId: 2, ambulanceId: 102, hospitalId: 1, pickupDist: 3, hospitalDist: 2 },
  { time: '14:07:44', emergencyId: 3, ambulanceId: 101, hospitalId: 1, pickupDist: 8, hospitalDist: 6 },
];

// Triage-style severity banding, same convention real EMS/ER systems use
// (red / orange / yellow / green) so operators can scan priority at a glance.
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
