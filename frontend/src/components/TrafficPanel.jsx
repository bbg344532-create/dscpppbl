import { useState } from 'react';
import { roadKey } from '../format';

// Jam a road (raise its cost) or clear a jam. Trips already under way are re-planned by the engine.
function TrafficPanel({ roads, selectedRoad, busy, onSelect, onApply }) {
  const road = roads.find((r) => roadKey(r) === selectedRoad) || null;
  const [typed, setTyped] = useState({ key: null, value: '' });
  const costValue = road ? (typed.key === selectedRoad ? typed.value : String(road.cost > road.baseCost ? road.cost : road.baseCost * 4)) : '';
  const jammed = roads.filter((r) => r.cost !== r.baseCost);
  const cost = Number(costValue);
  const valid = road && Number.isFinite(cost) && cost > 0;

  return (
    <section className="panel" aria-labelledby="traffic-heading">
      <div className="panel-header">
        <h2 id="traffic-heading">Traffic</h2>
        <span className="panel-count">{jammed.length === 0 ? 'no jams' : `${jammed.length} jammed`}</span>
      </div>
      <div className="traffic-form">
        <label className="field traffic-road">
          <span className="field-label">Road (or click one on the map)</span>
          <select aria-label="Road" value={selectedRoad || ''} onChange={(e) => onSelect(e.target.value || null)}>
            <option value="">Choose a road…</option>
            {roads.map((r) => <option key={roadKey(r)} value={roadKey(r)}>{r.fromNode} – {r.toNode} · cost {r.cost}{r.cost !== r.baseCost ? ` (normal ${r.baseCost})` : ''}</option>)}
          </select>
        </label>
        <label className="field traffic-cost">
          <span className="field-label">New cost</span>
          <input aria-label="New cost" type="number" min="1" step="1" value={costValue} disabled={!road}
            onChange={(e) => setTyped({ key: selectedRoad, value: e.target.value })} />
        </label>
        <button className="btn-primary" disabled={busy || !valid} onClick={() => onApply(road.fromNode, road.toNode, cost)}>Apply</button>
      </div>
      {jammed.length > 0 && (
        <ul className="jam-list">
          {jammed.map((r) => (
            <li key={roadKey(r)} data-testid="jam-item">
              <span>Road {r.fromNode}–{r.toNode}: cost {r.cost} (normal {r.baseCost})</span>
              <button className="btn-ghost btn-small" disabled={busy} onClick={() => onApply(r.fromNode, r.toNode, r.baseCost)}>Clear</button>
            </li>
          ))}
        </ul>
      )}
    </section>
  );
}

export default TrafficPanel;
