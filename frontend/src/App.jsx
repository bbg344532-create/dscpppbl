import Dashboard from './components/Dashboard';
import ConnectionStatus from './components/ConnectionStatus';
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
        <ConnectionStatus />
      </header>
      <main>
        <Dashboard />
      </main>
    </div>
  );
}

export default App;
