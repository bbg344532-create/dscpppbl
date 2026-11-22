// What the engine just did and why. The decisions come first (winners, then skipped cases, then the
// engine's own log); the full list of scores it compared is one click away.
function DispatchReasoning({ run }) {
  const winners = run.trace.filter((l) => l.startsWith('=>'));
  const skips = run.trace.filter((l) => l.startsWith('skip'));
  const scores = run.trace.filter((l) => !l.startsWith('=>') && !l.startsWith('skip'));
  const empty = run.trace.length === 0 && run.log.length === 0;
  return (
    <section className="panel reasoning-panel" aria-labelledby="reasoning-heading">
      <div className="panel-header">
        <h2 id="reasoning-heading">Dispatcher reasoning</h2>
        <span className="panel-count">{run.label || 'last action'}</span>
      </div>
      {empty ? (
        <p className="empty-state">Run a dispatch cycle or take an action to see the engine's reasoning here.</p>
      ) : (
        <div className="reasoning-feed" data-testid="reasoning">
          {winners.map((line, i) => <div key={`w${i}`} className="reasoning-line reasoning-win">{line}</div>)}
          {skips.map((line, i) => <div key={`s${i}`} className="reasoning-line reasoning-skip">{line}</div>)}
          {run.log.map((line, i) => <div key={`l${i}`} className="reasoning-line reasoning-log">{line}</div>)}
          {scores.length > 0 && (
            <details className="reasoning-details">
              <summary>All {scores.length} ambulance / emergency scores compared (severity × 10 − distance)</summary>
              {scores.map((line, i) => <div key={`c${i}`} className="reasoning-line">{line}</div>)}
            </details>
          )}
        </div>
      )}
    </section>
  );
}

export default DispatchReasoning;
