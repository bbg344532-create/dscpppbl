import { useState } from 'react';
import EmergencyList from './EmergencyList';
import AmbulanceStatus from './AmbulanceStatus';
import HospitalList from './HospitalList';
import MapView from './MapView';
import ReportEmergencyForm from './ReportEmergencyForm';
import DispatchReasoning from './DispatchReasoning';
import {
  initialHospitals,
  initialAmbulances,
  initialEmergencies,
  initialAssignedHospitalByAmbulance,
  initialDispatchLog,
} from '../data/mockData';
import { lookupType } from '../engine/severityTable';
import { runDispatchCycle, completeDispatch } from '../engine/dispatcher';

let nextEmergencyId = initialEmergencies.length + 1;
let nextOrder = initialEmergencies.length + 1;

// Dashboard runs the actual ported Dispatcher algorithm (engine/dispatcher.js)
// against local React state. Nothing here is faked: severity/specialization
// come from the real SeverityTable lookup, distances come from a real
// Dijkstra over the real weighted graph, and the matching logic is the same
// feasibility-skip + severity/distance scoring as the C++ Dispatcher.
function Dashboard() {
  const [ambulances, setAmbulances] = useState(initialAmbulances);
  const [hospitals, setHospitals] = useState(initialHospitals);
  const [emergencies, setEmergencies] = useState(initialEmergencies);
  const [assignedHospitalByAmbulance, setAssignedHospitalByAmbulance] = useState(initialAssignedHospitalByAmbulance);
  const [log, setLog] = useState(initialDispatchLog);
  const [trace, setTrace] = useState([]);

  function currentState() {
    return { ambulances, hospitals, emergencies, assignedHospitalByAmbulance };
  }

  function applyResult(result) {
    setAmbulances(result.ambulances);
    setHospitals(result.hospitals);
    setEmergencies(result.emergencies);
    setAssignedHospitalByAmbulance(result.assignedHospitalByAmbulance);
    setTrace(result.trace);
    if (result.newLogEntries.length > 0) {
      setLog((prev) => [...result.newLogEntries.reverse(), ...prev]);
    }
  }

  function handleRunDispatchCycle() {
    const result = runDispatchCycle(currentState());
    applyResult(result);
  }

  function handleCompleteDispatch(ambulanceId) {
    const result = completeDispatch(ambulanceId, currentState());
    applyResult(result);
  }

  function handleReportEmergency({ locationId, type }) {
    const resolved = lookupType(type);
    const time = new Date().toTimeString().slice(0, 8);
    const newEmergency = {
      id: nextEmergencyId++,
      locationId,
      type,
      severity: resolved.severity,
      requiredSpecialization: resolved.requiredSpecialization,
      reportedAt: time,
      order: nextOrder++,
    };
    setEmergencies((prev) => [...prev, newEmergency]);
  }

  const freeAmbulanceCount = ambulances.filter((a) => a.status === 'FREE').length;
  const latestPath = log.length > 0 ? log[0].path : null;

  return (
    <div className="dashboard-grid">
      <div className="col-left">
        <ReportEmergencyForm onReport={handleReportEmergency} />
        <EmergencyList emergencies={emergencies} />
      </div>

      <div className="col-center">
        <div className="dispatch-action-bar">
          <button className="btn-primary" onClick={handleRunDispatchCycle}>
            Run dispatch cycle
          </button>
          <span className="dispatch-action-hint">
            {freeAmbulanceCount} ambulance{freeAmbulanceCount !== 1 ? 's' : ''} free · {emergencies.length} waiting
          </span>
        </div>

        <MapView hospitals={hospitals} ambulances={ambulances} emergencies={emergencies} highlightPath={latestPath} />

        <DispatchReasoning trace={trace} />

        <section className="panel log-panel" aria-labelledby="log-heading">
          <div className="panel-header">
            <h2 id="log-heading">Dispatch log</h2>
            <span className="panel-count">{log.length} entries</span>
          </div>
          {log.length === 0 ? (
            <p className="empty-state">No dispatches yet.</p>
          ) : (
            <div className="log-feed">
              {log.map((entry, i) => (
                <div key={i} className="log-line">
                  <span className="log-time">{entry.time}</span>
                  <span className="log-text">
                    EMG-{String(entry.emergencyId).padStart(3, '0')} → AMB-{entry.ambulanceId} → HOSP-{entry.hospitalId}
                    <span className="log-dist"> (pickup {entry.pickupDist}, hospital {entry.hospitalDist})</span>
                  </span>
                </div>
              ))}
            </div>
          )}
        </section>
      </div>

      <div className="col-right">
        <AmbulanceStatus ambulances={ambulances} onCompleteDispatch={handleCompleteDispatch} />
        <HospitalList hospitals={hospitals} />
      </div>
    </div>
  );
}

export default Dashboard;
