import { timeOf } from '../format';

function DispatchLog({ history }) {
  return (
    <section className="panel log-panel" aria-labelledby="log-heading">
      <div className="panel-header">
        <h2 id="log-heading">Dispatch log</h2>
        <span className="panel-count">{history.length} entries</span>
      </div>
      {history.length === 0 ? (
        <p className="empty-state">No dispatches yet.</p>
      ) : (
        <div className="log-feed">
          {history.map((h) => (
            <div key={h.id} className="log-line" data-testid="log-entry">
              <span className="log-time">{timeOf(h.dispatchedAt)}</span>
              <span className="log-text">
                EMG-{String(h.emergencyId).padStart(3, '0')} {h.type} → AMB-{h.ambulanceId} → HOSP-{h.hospitalId}
                <span className="log-dist"> (pickup {h.pickupDistance}, hospital {h.hospitalDistance}){h.completedAt ? ' · done' : ''}</span>
              </span>
            </div>
          ))}
        </div>
      )}
    </section>
  );
}

export default DispatchLog;
