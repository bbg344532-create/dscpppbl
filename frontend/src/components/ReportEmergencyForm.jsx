import { useState } from 'react';
import { severityBand } from '../format';

// Structured intake: pick where and what. Severity and required specialization come from the
// emergency type table in the database, never from typed text.
function ReportEmergencyForm({ nodes, emergencyTypes, busy, onReport }) {
  const [node, setNode] = useState('');
  const [type, setType] = useState('');
  const nodeValue = node || String(nodes.length ? nodes[0].id : '');
  const typeValue = type || (emergencyTypes.length ? emergencyTypes[0].type : '');
  const resolved = emergencyTypes.find((t) => t.type === typeValue);
  const band = resolved ? severityBand(resolved.severity) : null;

  function handleSubmit(e) {
    e.preventDefault();
    onReport(Number(nodeValue), typeValue);
  }

  return (
    <section className="panel report-panel" aria-labelledby="report-heading">
      <div className="panel-header"><h2 id="report-heading">Report emergency</h2></div>
      <form className="report-form" onSubmit={handleSubmit}>
        <label className="field">
          <span className="field-label">Location</span>
          <select aria-label="Location" value={nodeValue} onChange={(e) => setNode(e.target.value)}>
            {nodes.map((n) => <option key={n.id} value={n.id}>Node {n.id} · {n.name}</option>)}
          </select>
        </label>
        <label className="field">
          <span className="field-label">Emergency type</span>
          <select aria-label="Emergency type" value={typeValue} onChange={(e) => setType(e.target.value)}>
            {emergencyTypes.map((t) => <option key={t.type} value={t.type}>{t.type}</option>)}
          </select>
        </label>
        {resolved && (
          <div className="resolved-preview">
            <span>severity <strong style={{ color: band.color }}>{resolved.severity}</strong> ({band.label})</span>
            <span>needs <strong>{resolved.requiredSpecialization}</strong></span>
          </div>
        )}
        <button type="submit" className="btn-primary" disabled={busy}>Add to queue</button>
      </form>
    </section>
  );
}

export default ReportEmergencyForm;
