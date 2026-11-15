function HospitalList({ hospitals, busy, onDischarge }) {
  return (
    <section className="panel" aria-labelledby="hospital-heading">
      <div className="panel-header">
        <h2 id="hospital-heading">Hospitals</h2>
        <span className="panel-count">{hospitals.length} in network</span>
      </div>
      <ul className="hospital-list">
        {hospitals.map((h) => {
          const occupancy = h.totalBeds ? 1 - h.availableBeds / h.totalBeds : 1;
          // Colour shows how close to full the hospital is (the same thing the Dispatcher weighs for load).
          const fillColor = occupancy >= 0.85 ? 'var(--sev-critical)' : occupancy >= 0.6 ? 'var(--sev-urgent)' : 'var(--status-free)';
          return (
            <li key={h.id} className="hospital-item" data-testid="hospital-item">
              <div className="hospital-top-row">
                <span className="hospital-name">{h.name}</span>
                <span className="hospital-beds">{h.availableBeds}/{h.totalBeds} beds</span>
              </div>
              <div className="hospital-bar-track" aria-hidden="true">
                <div className="hospital-bar-fill" style={{ width: `${occupancy * 100}%`, background: fillColor }} />
              </div>
              <div className="hospital-bottom-row">
                <div className="hospital-tags">
                  {h.specializations.map((s) => <span key={s} className="spec-tag">{s}</span>)}
                </div>
                <button className="btn-ghost btn-small" disabled={busy || h.availableBeds >= h.totalBeds}
                  onClick={() => onDischarge(h.id)} aria-label={`Discharge a patient at ${h.name}`}>Discharge</button>
              </div>
            </li>
          );
        })}
      </ul>
    </section>
  );
}

export default HospitalList;
