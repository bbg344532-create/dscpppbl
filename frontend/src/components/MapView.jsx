import { roadKey, severityBand, statusColor } from '../format';

// Schematic map drawn from the database's nodes and roads (positions are layout positions, not GPS).
// Orange roads are jammed (cost above normal); the thick teal line is the route of the latest dispatch;
// thin teal lines are trips still in progress. Click a road to select it for a traffic change.
function MapView({ nodes, roads, hospitals, ambulances, waiting, tripRoutes, newRoutes, selectedRoad, onSelectRoad }) {
  const pos = {};
  nodes.forEach((n) => { pos[n.id] = n; });
  const xs = nodes.map((n) => n.x); const ys = nodes.map((n) => n.y);
  const minX = Math.min(...xs) - 50; const minY = Math.min(...ys) - 50;
  const width = Math.max(...xs) + 60 - minX; const height = Math.max(...ys) + 50 - minY;

  const edgeSet = (routes) => {
    const set = new Set();
    routes.forEach((r) => { for (let i = 0; i < r.length - 1; i += 1) { set.add(`${r[i]}-${r[i + 1]}`); set.add(`${r[i + 1]}-${r[i]}`); } });
    return set;
  };
  const onTrip = edgeSet(tripRoutes); const fresh = edgeSet(newRoutes);

  const slot = (list, nodeOf) => { // spread several markers that share one node
    const seen = {};
    return list.map((item) => { const n = nodeOf(item); seen[n] = (seen[n] || 0) + 1; return { item, index: seen[n] - 1 }; });
  };

  return (
    <section className="panel map-panel" aria-labelledby="map-heading">
      <div className="panel-header">
        <h2 id="map-heading">City map</h2>
        <span className="panel-count">{nodes.length} nodes · {roads.length} roads</span>
      </div>
      <svg viewBox={`${minX} ${minY} ${width} ${height}`} className="map-svg" role="img" aria-label="Road network with hospitals, ambulances and emergencies">
        {roads.map((r) => {
          const a = pos[r.fromNode]; const b = pos[r.toNode]; const key = roadKey(r);
          const jammed = r.cost > r.baseCost;
          const cls = fresh.has(key) ? 'map-edge map-edge-highlight' : jammed ? 'map-edge map-edge-jam' : onTrip.has(key) ? 'map-edge map-edge-trip' : 'map-edge';
          return (
            <g key={key}>
              <line x1={a.x} y1={a.y} x2={b.x} y2={b.y} className={`${cls}${selectedRoad === key ? ' map-edge-selected' : ''}`} />
              <line x1={a.x} y1={a.y} x2={b.x} y2={b.y} className="map-edge-hit" data-testid="road" onClick={() => onSelectRoad(key)}>
                <title>Road {r.fromNode}–{r.toNode}: cost {r.cost}{jammed ? ` (normal ${r.baseCost})` : ''}</title>
              </line>
            </g>
          );
        })}
        {nodes.map((n) => (
          <g key={n.id}>
            {n.type === 'DEPOT'
              ? <rect x={n.x - 4} y={n.y - 4} width="8" height="8" className="map-depot" transform={`rotate(45 ${n.x} ${n.y})`}><title>Ambulance depot: {n.name}</title></rect>
              : <circle cx={n.x} cy={n.y} r="4" className="map-node"><title>Node {n.id}: {n.name}</title></circle>}
            <text x={n.x} y={n.y - 9} className="map-node-label">{n.id}</text>
          </g>
        ))}
        {hospitals.map((h) => {
          const n = pos[h.node];
          return (
            <g key={`h${h.id}`} transform={`translate(${n.x}, ${n.y})`}>
              <circle r="9" className="map-hospital-circle" />
              <path d="M -4 0 H 4 M 0 -4 V 4" className="map-hospital-cross" />
              <title>{h.name}: {h.availableBeds}/{h.totalBeds} beds free · {h.specializations.join(', ')}</title>
            </g>
          );
        })}
        {slot(ambulances, (a) => a.node).map(({ item: a, index }) => {
          const n = pos[a.node];
          return (
            <g key={`a${a.id}`} transform={`translate(${n.x + 14 + index * 13}, ${n.y - 14})`}>
              <rect x="-6" y="-6" width="12" height="12" rx="3" fill={statusColor(a.status)} />
              <title>AMB-{a.id}: {a.status} at node {a.node}</title>
            </g>
          );
        })}
        {slot(waiting, (e) => e.node).map(({ item: e, index }) => {
          const n = pos[e.node]; const band = severityBand(e.severity);
          return (
            <g key={`e${e.id}`} transform={`translate(${n.x - 14 - index * 13}, ${n.y + 14})`}>
              <circle r="7" fill="none" stroke={band.color} strokeWidth="2" />
              <circle r="2.5" fill={band.color} />
              <title>EMG-{e.id} {e.type}: severity {e.severity} at node {e.node}</title>
            </g>
          );
        })}
      </svg>
      <div className="map-legend">
        <span><span className="legend-swatch legend-hospital" /> Hospital</span>
        <span><span className="legend-swatch legend-ambulance" /> Ambulance</span>
        <span><span className="legend-swatch legend-emergency" /> Emergency</span>
        <span><span className="legend-swatch legend-depot" /> Depot</span>
        <span><span className="legend-swatch legend-route" /> Latest route</span>
        <span><span className="legend-swatch legend-jam" /> Traffic jam</span>
      </div>
    </section>
  );
}

export default MapView;
