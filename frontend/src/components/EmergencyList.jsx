import { severityBand } from '../data/mockData';

// Pure display of the Priority Queue's current order (severity desc, then
// earliest-reported first) — no per-row action here, since in the real
// engine individual emergencies are never dispatched one at a time by
// choice; the Dispatcher decides which to serve via runDispatchCycle.
function EmergencyList({ emergencies }) {
  const sorted = [...emergencies].sort(
    (a, b) => (b.severity - a.severity) || (a.order - b.order)
  );

  return (
    <section className="panel" aria-labelledby="queue-heading">
      <div className="panel-header">
        <h2 id="queue-heading">Emergency queue</h2>
        <span className="panel-count">{emergencies.length} waiting</span>
      </div>

      {sorted.length === 0 ? (
        <p className="empty-state">No emergencies waiting. All clear.</p>
      ) : (
        <ul className="queue-list">
          {sorted.map((e, i) => {
            const band = severityBand(e.severity);
            return (
              <li key={e.id} className="queue-item" style={{ '--band-color': band.color }}>
                <span className="queue-rank" aria-hidden="true">{i + 1}</span>
                <span className="queue-severity">{e.severity}</span>
                <div className="queue-body">
                  <span className="queue-type">EMG-{e.id} · {e.type}</span>
                  <div className="queue-meta">
                    <span className="queue-band">{band.label}</span>
                    <span>loc {e.locationId}</span>
                    <span>needs {e.requiredSpecialization}</span>
                    <span>{e.reportedAt}</span>
                  </div>
                </div>
              </li>
            );
          })}
        </ul>
      )}
    </section>
  );
}

export default EmergencyList;
