import { nodePositions, edgesForDisplay } from '../engine/graph';
import { severityBand, statusColor } from '../data/mockData';

// Schematic network map — the real node/edge graph the Dijkstra module
// operates on (same one core-engine/src/main.cpp uses), not a generic map
// embed. When a dispatch happens, its actual computed route (ambulance ->
// patient -> hospital) is highlighted here, so the routing decision is
// visible, not just logged as text.
function MapView({ hospitals, ambulances, emergencies, highlightPath }) {
  const highlightEdges = new Set();
  if (highlightPath && highlightPath.length > 1) {
    for (let i = 0; i < highlightPath.length - 1; i++) {
      highlightEdges.add(`${highlightPath[i]}-${highlightPath[i + 1]}`);
      highlightEdges.add(`${highlightPath[i + 1]}-${highlightPath[i]}`);
    }
  }

  return (
    <section className="panel map-panel" aria-labelledby="map-heading">
      <div className="panel-header">
        <h2 id="map-heading">Network map</h2>
        <span className="panel-count">schematic — node graph</span>
      </div>

      <svg viewBox="0 0 800 460" className="map-svg" role="img" aria-label="Schematic road network with hospitals, ambulances, and emergencies">
        {edgesForDisplay.map(([a, b], i) => {
          const pa = nodePositions[a];
          const pb = nodePositions[b];
          const isHighlighted = highlightEdges.has(`${a}-${b}`);
          return (
            <line
              key={i}
              x1={pa.x} y1={pa.y} x2={pb.x} y2={pb.y}
              className={isHighlighted ? 'map-edge map-edge-highlight' : 'map-edge'}
            />
          );
        })}

        {Object.entries(nodePositions).map(([id, pos]) => (
          <g key={id}>
            <circle cx={pos.x} cy={pos.y} r="4" className="map-node" />
            <text x={pos.x} y={pos.y - 10} className="map-node-label">{id}</text>
          </g>
        ))}

        {hospitals.map((h) => {
          const pos = nodePositions[h.locationId];
          return (
            <g key={`h${h.id}`} transform={`translate(${pos.x}, ${pos.y})`}>
              <circle r="9" className="map-hospital-circle" />
              <path d="M -4 0 H 4 M 0 -4 V 4" className="map-hospital-cross" />
              <title>{h.name} — {h.availableBeds}/{h.totalBeds} beds — {h.specializations.join(', ')}</title>
            </g>
          );
        })}

        {ambulances.map((a) => {
          const pos = nodePositions[a.locationId];
          return (
            <g key={`a${a.id}`} transform={`translate(${pos.x + 14}, ${pos.y - 14})`}>
              <rect x="-6" y="-6" width="12" height="12" rx="3" fill={statusColor(a.status)} />
              <title>AMB-{a.id} — {a.status} — loc {a.locationId}</title>
            </g>
          );
        })}

        {emergencies.map((e) => {
          const pos = nodePositions[e.locationId];
          const band = severityBand(e.severity);
          return (
            <g key={`e${e.id}`} transform={`translate(${pos.x - 14}, ${pos.y + 14})`}>
              <circle r="7" fill="none" stroke={band.color} strokeWidth="2" />
              <circle r="2.5" fill={band.color} />
              <title>{e.type} — severity {e.severity} — loc {e.locationId}</title>
            </g>
          );
        })}
      </svg>

      <div className="map-legend">
        <span><span className="legend-swatch legend-hospital" /> Hospital</span>
        <span><span className="legend-swatch legend-ambulance" /> Ambulance</span>
        <span><span className="legend-swatch legend-emergency" /> Emergency</span>
        <span><span className="legend-swatch legend-route" /> Last route</span>
      </div>
    </section>
  );
}

export default MapView;
