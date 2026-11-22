// Small display helpers shared by the components.

// Triage-style severity bands (the red / orange / yellow / green used on real ER triage tags).
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

export const statusLabel = { FREE: 'Available', EN_ROUTE: 'En route', OCCUPIED: 'Occupied' };

export const roadKey = (road) => `${road.fromNode}-${road.toNode}`;

// "2026-01-01 14:03:55" -> "14:03:55"
export const timeOf = (stamp) => (stamp ? String(stamp).slice(11, 19) : '');
