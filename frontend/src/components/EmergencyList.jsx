import { severityBand } from '../format';

// The waiting emergencies, most urgent first (the order the backend already sorted them in).
function EmergencyList({ waiting, nodes }) {
  const nameOf = (id) => (nodes.find((n) => n.id === id) || {}).name || '';
  return (
    <section className="panel" aria-labelledby="queue-heading">
      <div className="panel-header">
        <h2 id="queue-heading">Emergency queue</h2>
        <span className="panel-count">{waiting.length} waiting</span>
      </div>
      {waiting.length === 0 ? (
        <p className="empty-state">No emergencies waiting. All clear.</p>
      ) : (
        <ul className="queue-list">
          {waiting.map((e, i) => {
            const band = severityBand(e.severity);
            return (
              <li key={e.id} className="queue-item" data-testid="waiting-item" style={{ '--band-color': band.color }}>
                <span className="queue-rank" aria-hidden="true">{i + 1}</span>
                <span className="queue-severity">{e.severity}</span>
                <div className="queue-body">
                  <span className="queue-type">EMG-{e.id} · {e.type}</span>
                  <div className="queue-meta">
                    <span className="queue-band">{band.label}</span>
                    <span>node {e.node}{nameOf(e.node) ? ` · ${nameOf(e.node)}` : ''}</span>
                    <span>needs {e.requiredSpecialization}</span>
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
