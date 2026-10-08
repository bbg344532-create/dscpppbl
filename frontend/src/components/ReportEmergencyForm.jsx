import { useState } from 'react';
import { nodePositions } from '../engine/graph';
import { EMERGENCY_TYPES, lookupType } from '../engine/severityTable';
import { severityBand } from '../data/mockData';

const locationIds = Object.keys(nodePositions).map(Number);

// Structured intake, not free text: the operator picks a location and a
// TYPE, and severity + required specialization are resolved automatically
// from SeverityTable — same design as Dispatcher::reportEmergency in the
// core engine, which never accepts a raw severity number from the caller.
function ReportEmergencyForm({ onReport }) {
  const [locationId, setLocationId] = useState(locationIds[0]);
  const [type, setType] = useState(EMERGENCY_TYPES[0].type);

  const resolved = lookupType(type);
  const band = severityBand(resolved.severity);

  function handleSubmit(e) {
    e.preventDefault();
    onReport({ locationId: Number(locationId), type });
  }

  return (
    <section className="panel report-panel" aria-labelledby="report-heading">
      <div className="panel-header">
        <h2 id="report-heading">Report emergency</h2>
      </div>
      <form className="report-form" onSubmit={handleSubmit}>
        <label className="field">
          <span className="field-label">Location (node)</span>
          <select value={locationId} onChange={(e) => setLocationId(e.target.value)}>
            {locationIds.map((id) => (
              <option key={id} value={id}>Node {id}</option>
            ))}
          </select>
        </label>

        <label className="field">
          <span className="field-label">Emergency type</span>
          <select value={type} onChange={(e) => setType(e.target.value)}>
            {EMERGENCY_TYPES.map((t) => (
              <option key={t.type} value={t.type}>{t.type}</option>
            ))}
          </select>
        </label>

        <div className="resolved-preview">
          <span>severity <strong style={{ color: band.color }}>{resolved.severity}</strong> ({band.label})</span>
          <span>needs <strong>{resolved.requiredSpecialization}</strong></span>
        </div>

        <button type="submit" className="btn-primary">Add to queue</button>
      </form>
    </section>
  );
}

export default ReportEmergencyForm;
