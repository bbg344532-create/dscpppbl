import { useEffect, useState } from 'react';
import { api } from '../api';

// Shows whether the backend (and its engine program) can be reached. Checks every 8 seconds.
function ConnectionStatus() {
  const [kind, setKind] = useState('checking');

  useEffect(() => {
    let alive = true;
    async function check() {
      try {
        const health = await api.health();
        if (alive) setKind(health.engine ? 'online' : 'noengine');
      } catch {
        if (alive) setKind('offline');
      }
    }
    check();
    const timer = setInterval(check, 8000);
    return () => { alive = false; clearInterval(timer); };
  }, []);

  const text = {
    checking: 'Connecting…',
    online: 'Connected to backend',
    noengine: 'Backend running, engine program missing',
    offline: 'Backend offline',
  }[kind];

  return (
    <div className="app-status" data-testid="connection">
      <span className={`live-dot live-${kind}`} aria-hidden="true" />
      {text}
    </div>
  );
}

export default ConnectionStatus;
