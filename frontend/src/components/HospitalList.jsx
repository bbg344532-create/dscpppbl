function HospitalList({ hospitals }) {
  return (
    <section className="panel" aria-labelledby="hospital-heading">
      <div className="panel-header">
        <h2 id="hospital-heading">Hospitals</h2>
        <span className="panel-count">{hospitals.length} in network</span>
      </div>
      <ul className="hospital-list">
        {hospitals.map((h) => {
          const occupancy = 1 - h.beds / h.totalBeds;
          // Color reflects how close the hospital is to capacity, same
          // logic the Dispatcher itself uses for load-balancing decisions.
          const fillColor =
            occupancy >= 0.85 ? 'var(--sev-critical)' :
            occupancy >= 0.6 ? 'var(--sev-urgent)' :
            'var(--status-free)';
          return (
            <li key={h.id} className="hospital-item">
              <div className="hospital-top-row">
                <span className="hospital-name">{h.name}</span>
                <span className="hospital-beds">{h.beds}/{h.totalBeds} beds</span>
              </div>
              <div className="hospital-bar-track" aria-hidden="true">
                <div
                  className="hospital-bar-fill"
                  style={{ width: `${occupancy * 100}%`, background: fillColor }}
                />
              </div>
              <div className="hospital-tags">
                {h.specializations.map((s) => (
                  <span key={s} className="spec-tag">{s}</span>
                ))}
              </div>
            </li>
          );
        })}
      </ul>
    </section>
  );
}

export default HospitalList;
