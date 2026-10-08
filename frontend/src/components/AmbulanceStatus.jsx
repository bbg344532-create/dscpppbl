import { statusColor } from '../data/mockData';

function AmbulanceStatus({ ambulances, onCompleteDispatch }) {
  const freeCount = ambulances.filter((a) => a.status === 'FREE').length;

  return (
    <section className="panel" aria-labelledby="ambulance-heading">
      <div className="panel-header">
        <h2 id="ambulance-heading">Ambulances</h2>
        <span className="panel-count">{freeCount} free of {ambulances.length}</span>
      </div>
      <ul className="status-list">
        {ambulances.map((a) => (
          <li key={a.id} className="status-item">
            <span className="status-dot" style={{ background: statusColor(a.status) }} aria-hidden="true" />
            <span className="status-id">AMB-{a.id}</span>
            <span className="status-loc">loc {a.locationId}</span>
            <span className="status-tag" style={{ color: statusColor(a.status) }}>{a.status}</span>
            {a.status === 'EN_ROUTE' && (
              <button className="btn-ghost btn-small" onClick={() => onCompleteDispatch(a.id)}>
                Complete
              </button>
            )}
          </li>
        ))}
      </ul>
    </section>
  );
}

export default AmbulanceStatus;
