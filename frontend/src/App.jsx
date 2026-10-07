import Dashboard from './components/Dashboard';
import './styles/tokens.css';
import './styles/App.css';

function App() {
  return (
    <div className="app-shell">
      <header className="app-header">
        <div className="app-title-group">
          <span className="app-icon" aria-hidden="true">🚑</span>
          <div>
            <h1 className="app-title">MedRoute</h1>
            <p className="app-subtitle">Emergency Hospital Routing &amp; Resource Optimization</p>
          </div>
        </div>
        <div className="app-status">
          <span className="live-dot" aria-hidden="true" />
          Simulated data — not yet connected to backend
        </div>
      </header>

      <main>
        <Dashboard />
      </main>
    </div>
  );
}

export default App;
