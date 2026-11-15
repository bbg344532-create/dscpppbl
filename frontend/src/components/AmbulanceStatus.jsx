import { statusColor, statusLabel } from '../format';

function AmbulanceStatus({ ambulances, trips, hospitals, busy, onComplete }) {
  const freeCount = ambulances.filter((a) => a.status === 'FREE').length;
  const hospitalName = (id) => (hospitals.find((h) => h.id === id) || {}).name || `Hospital ${id}`;

  return (
    <section className="panel" aria-labelledby="ambulance-heading">
      <div className="panel-header">
        <h2 id="ambulance-heading">Ambulances</h2>
        <span className="panel-count">{freeCount} free of {ambulances.length}</span>
      </div>
      <ul className="status-list">
        {ambulances.map((a) => {
          const trip = trips.find((t) => t.ambulanceId === a.id);
          return (
            <li key={a.id} className="status-item status-item-tall" data-testid="ambulance-item">
              <div className="status-row">
                <span className="status-dot" style={{ background: statusColor(a.status) }} aria-hidden="true" />
                <span className="status-id">AMB-{a.id}</span>
                <span className="status-loc">node {a.node}</span>
                <span className="status-tag" style={{ color: statusColor(a.status) }}>{statusLabel[a.status]}</span>
              </div>
              {a.status === 'EN_ROUTE' && (
                <div className="status-second">
                  <span className="trip-line">
                    {trip ? `EMG-${trip.emergencyId} ${trip.type} → ${hospitalName(trip.hospitalId)} · route cost ${trip.cost}` : 'No trip details'}
                  </span>
                  <button className="btn-ghost btn-small" disabled={busy} onClick={() => onComplete(a.id)}>Complete</button>
                </div>
              )}
            </li>
          );
        })}
      </ul>
    </section>
  );
}

export default AmbulanceStatus;
