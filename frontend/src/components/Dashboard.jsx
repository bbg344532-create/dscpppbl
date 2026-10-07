import { useState } from 'react';
import EmergencyList from './EmergencyList';
import AmbulanceStatus from './AmbulanceStatus';
import HospitalList from './HospitalList';
import MapView from './MapView';
import {
  hospitals as initialHospitals,
  ambulances as initialAmbulances,
  emergencies as initialEmergencies,
  dispatchLog as initialLog,
} from '../data/mockData';

// Dashboard composes all panels from local, hardcoded state. This is a
// Phase-II mock UI, intentionally not wired to the backend/DB yet — the
// "Dispatch" action below simulates what the core engine's Dispatcher does
// (move an emergency out of the queue into the log) purely in the browser,
// so the screen is interactive without needing a live connection.
function Dashboard() {
  const [emergencies, setEmergencies] = useState(initialEmergencies);
  const [log, setLog] = useState(initialLog);
  const [ambulances] = useState(initialAmbulances);
  const [hospitals] = useState(initialHospitals);

  function handleSimulatedDispatch(emergencyId) {
    const emergency = emergencies.find((e) => e.id === emergencyId);
    const freeAmbulance = ambulances.find((a) => a.status === 'FREE');
    if (!emergency || !freeAmbulance) return;

    const now = new Date();
    const time = now.toTimeString().slice(0, 8);

    setLog((prev) => [
      { time, emergencyId: emergency.id, ambulanceId: freeAmbulance.id, hospitalId: hospitals[0].id, pickupDist: '—', hospitalDist: '—' },
      ...prev,
    ]);
    setEmergencies((prev) => prev.filter((e) => e.id !== emergencyId));
  }

  return (
    <div className="dashboard-grid">
      <div className="col-left">
        <EmergencyList emergencies={emergencies} onDispatch={handleSimulatedDispatch} />
      </div>

      <div className="col-center">
        <MapView hospitals={hospitals} ambulances={ambulances} emergencies={emergencies} />
        <section className="panel log-panel" aria-labelledby="log-heading">
          <div className="panel-header">
            <h2 id="log-heading">Dispatch log</h2>
            <span className="panel-count">{log.length} entries</span>
          </div>
          <div className="log-feed">
            {log.map((entry, i) => (
              <div key={i} className="log-line">
                <span className="log-time">{entry.time}</span>
                <span className="log-text">
                  EMG-{String(entry.emergencyId).padStart(3, '0')} → AMB-{entry.ambulanceId} → HOSP-{entry.hospitalId}
                  {typeof entry.pickupDist === 'number' && (
                    <span className="log-dist"> (pickup {entry.pickupDist}, hospital {entry.hospitalDist})</span>
                  )}
                </span>
              </div>
            ))}
          </div>
        </section>
      </div>

      <div className="col-right">
        <AmbulanceStatus ambulances={ambulances} />
        <HospitalList hospitals={hospitals} />
      </div>
    </div>
  );
}

export default Dashboard;
