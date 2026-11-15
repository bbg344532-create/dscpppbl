import { useCallback, useEffect, useRef, useState } from 'react';
import { api } from '../api';
import ReportEmergencyForm from './ReportEmergencyForm';
import EmergencyList from './EmergencyList';
import AmbulanceStatus from './AmbulanceStatus';
import HospitalList from './HospitalList';
import MapView from './MapView';
import TrafficPanel from './TrafficPanel';
import ScenarioPanel from './ScenarioPanel';
import DispatchReasoning from './DispatchReasoning';
import DispatchLog from './DispatchLog';

const NO_RUN = { label: '', trace: [], log: [], routes: [] };

// All data lives in the backend's database. This component asks for it, shows it, and sends the
// operator's actions; every dispatch decision is made by the C++ engine behind the backend.
function Dashboard() {
  const [state, setState] = useState(null);
  const [loadError, setLoadError] = useState(null);
  const [notice, setNotice] = useState(null);
  const [busy, setBusy] = useState(false);
  const [run, setRun] = useState(NO_RUN);
  const [selectedRoad, setSelectedRoad] = useState(null);
  const busyRef = useRef(false);

  const refresh = useCallback(async () => {
    try { setState(await api.getState()); setLoadError(null); } catch (e) { setLoadError(e.message); }
  }, []);

  useEffect(() => {
    let alive = true;
    const load = async () => {
      try {
        const latest = await api.getState();
        if (alive) { setState(latest); setLoadError(null); }
      } catch (e) {
        if (alive) setLoadError(e.message);
      }
    };
    load();
    const timer = setInterval(() => { if (!busyRef.current) load(); }, 8000);
    return () => { alive = false; clearInterval(timer); };
  }, []);

  // Runs one backend call, then shows the new state and the engine's reasoning (or the error).
  async function perform(label, call) {
    busyRef.current = true; setBusy(true); setNotice(null);
    try {
      const result = await call();
      setState(result.state); setLoadError(null);
      setRun({ label, trace: result.trace || [], log: result.log || [], routes: (result.dispatches || []).map((d) => d.route) });
    } catch (e) {
      setNotice(e.message);
    } finally {
      busyRef.current = false; setBusy(false);
    }
  }

  if (!state) {
    return loadError ? (
      <div className="panel error-panel" role="alert">
        <h2>Cannot load data</h2>
        <p>{loadError}</p>
        <p>Start the backend (<code>npm start</code> in the <code>backend</code> folder), then try again.</p>
        <button className="btn-primary" onClick={refresh}>Try again</button>
      </div>
    ) : <p className="loading">Loading…</p>;
  }

  const freeCount = state.ambulances.filter((a) => a.status === 'FREE').length;

  return (
    <div>
      {loadError && <div className="banner banner-warn" role="status">Connection lost: showing the last data received. {loadError}</div>}
      {notice && (
        <div className="banner banner-error" role="alert" data-testid="notice">
          <span>{notice}</span>
          <button className="btn-ghost btn-small" onClick={() => setNotice(null)}>Dismiss</button>
        </div>
      )}
      <div className="dashboard-grid">
        <div className="col-left">
          <ReportEmergencyForm nodes={state.nodes} emergencyTypes={state.emergencyTypes} busy={busy}
            onReport={(node, type) => perform('Report emergency', () => api.reportEmergency(node, type))} />
          <EmergencyList waiting={state.waiting} nodes={state.nodes} />
          <ScenarioPanel scenarios={state.scenarios} busy={busy}
            onLoad={(id) => { setSelectedRoad(null); perform('Load scenario', () => api.loadScenario(id)); }}
            onReset={() => { setSelectedRoad(null); perform('Reset', () => api.reset()); }} />
        </div>
        <div className="col-center">
          <div className="dispatch-action-bar">
            <button className="btn-primary" disabled={busy} onClick={() => perform('Run dispatch cycle', () => api.runDispatch())}>Run dispatch cycle</button>
            <span className="dispatch-action-hint">{freeCount} ambulance{freeCount !== 1 ? 's' : ''} available · {state.waiting.length} waiting</span>
          </div>
          <MapView nodes={state.nodes} roads={state.roads} hospitals={state.hospitals} ambulances={state.ambulances}
            waiting={state.waiting} tripRoutes={state.trips.map((t) => t.route)} newRoutes={run.routes}
            selectedRoad={selectedRoad} onSelectRoad={setSelectedRoad} />
          <TrafficPanel roads={state.roads} selectedRoad={selectedRoad} busy={busy} onSelect={setSelectedRoad}
            onApply={(from, to, cost) => perform('Traffic change', () => api.setTraffic(from, to, cost))} />
          <DispatchReasoning run={run} />
          <DispatchLog history={state.history} />
        </div>
        <div className="col-right">
          <AmbulanceStatus ambulances={state.ambulances} trips={state.trips} hospitals={state.hospitals} busy={busy}
            onComplete={(id) => perform('Complete trip', () => api.completeTrip(id))} />
          <HospitalList hospitals={state.hospitals} busy={busy}
            onDischarge={(id) => perform('Discharge patient', () => api.discharge(id))} />
        </div>
      </div>
    </div>
  );
}

export default Dashboard;
