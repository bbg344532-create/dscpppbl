import { graphNodes, graphEdges, severityBand, statusColor } from '../data/mockData';

// Schematic network map — NOT a real-world map. It renders the exact same
// node/edge graph the C++ core engine's Graph class operates on
// (core-engine/src/main.cpp), with hospitals, ambulances, and emergencies
// plotted at their actual location_id positions. This is deliberately tied
// to the real system rather than a generic map embed, since no live
// hospital/ambulance GPS data exists for this prototype (see project docs).
function MapView({ hospitals, ambulances, emergencies }) {
  return (
    <section className="panel map-panel" aria-labelledby="map-heading">
      <div className="panel-header">
        <h2 id="map-heading">Network map</h2>
        <span className="panel-count">schematic — node graph</span>
      </div>

      <svg viewBox="0 0 800 460" className="map-svg" role="img" aria-label="Schematic road network with hospitals, ambulances, and emergencies">
        {/* Edges */}
        {graphEdges.map(([a, b], i) => {
          const pa = graphNodes[a];
          const pb = graphNodes[b];
          return (
            <line
              key={i}
              x1={pa.x} y1={pa.y} x2={pb.x} y2={pb.y}
              className="map-edge"
            />
          );
        })}

        {/* Base nodes */}
        {Object.entries(graphNodes).map(([id, pos]) => (
          <g key={id}>
            <circle cx={pos.x} cy={pos.y} r="4" className="map-node" />
            <text x={pos.x} y={pos.y - 10} className="map-node-label">{id}</text>
          </g>
        ))}

        {/* Hospitals */}
        {hospitals.map((h) => {
          const pos = graphNodes[h.locationId];
          return (
            <g key={`h${h.id}`} transform={`translate(${pos.x}, ${pos.y})`}>
              <circle r="9" className="map-hospital-circle" />
              <path d="M -4 0 H 4 M 0 -4 V 4" className="map-hospital-cross" />
              <title>{h.name} — {h.beds}/{h.totalBeds} beds — {h.specializations.join(', ')}</title>
            </g>
          );
        })}

        {/* Ambulances */}
        {ambulances.map((a) => {
          const pos = graphNodes[a.locationId];
          return (
            <g key={`a${a.id}`} transform={`translate(${pos.x + 14}, ${pos.y - 14})`}>
              <rect x="-6" y="-6" width="12" height="12" rx="3" fill={statusColor(a.status)} />
              <title>AMB-{a.id} — {a.status} — loc {a.locationId}</title>
            </g>
          );
        })}

        {/* Emergencies */}
        {emergencies.map((e) => {
          const pos = graphNodes[e.locationId];
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
      </div>
    </section>
  );
}

export default MapView;
