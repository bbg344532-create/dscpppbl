import { severityBand } from '../data/mockData';

// Queue of pending emergencies, ordered by severity — visually mirrors how
// the core engine's PriorityQueue (Max-Heap) actually orders them.
function EmergencyList({ emergencies, onDispatch }) {
  const sorted = [...emergencies].sort((a, b) => b.severity - a.severity);

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
          {sorted.map((e) => {
            const band = severityBand(e.severity);
            return (
              <li key={e.id} className="queue-item" style={{ '--band-color': band.color }}>
                <span className="queue-severity" aria-hidden="true">{e.severity}</span>
                <div className="queue-body">
                  <span className="queue-type">{e.type}</span>
                  <div className="queue-meta">
                    <span className="queue-band">{band.label}</span>
                    <span>loc {e.locationId}</span>
                    <span>needs {e.requiredSpecialization}</span>
                    <span>{e.reportedAt}</span>
                  </div>
                </div>
                {onDispatch && (
                  <button className="btn-ghost" onClick={() => onDispatch(e.id)}>
                    Dispatch
                  </button>
                )}
              </li>
            );
          })}
        </ul>
      )}
    </section>
  );
}

export default EmergencyList;
