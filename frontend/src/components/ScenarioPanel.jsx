import { useState } from 'react';

// Ready-made demo situations. Loading one resets all data to the starting data plus that scenario.
function ScenarioPanel({ scenarios, busy, onLoad, onReset }) {
  const [choice, setChoice] = useState('');
  const value = choice || String(scenarios.length ? scenarios[0].id : '');
  const chosen = scenarios.find((s) => String(s.id) === value);
  return (
    <section className="panel" aria-labelledby="scenario-heading">
      <div className="panel-header"><h2 id="scenario-heading">Demo scenarios</h2></div>
      <label className="field">
        <span className="field-label">Scenario</span>
        <select aria-label="Scenario" value={value} onChange={(e) => setChoice(e.target.value)}>
          {scenarios.map((s) => <option key={s.id} value={s.id}>{s.name}</option>)}
        </select>
      </label>
      {chosen && <p className="scenario-description">{chosen.description}</p>}
      <div className="button-row">
        <button className="btn-primary" disabled={busy || !chosen} onClick={() => onLoad(Number(value))}>Load scenario</button>
        <button className="btn-ghost" disabled={busy} onClick={onReset}>Reset all data</button>
      </div>
    </section>
  );
}

export default ScenarioPanel;
