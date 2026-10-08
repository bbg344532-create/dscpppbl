// Shows the Dispatcher's step-by-step reasoning from the most recent run:
// which candidates were considered, which were skipped (and why), their
// scores, and which one won. This is the part that actually demonstrates
// the algorithm, not just its end result.
function DispatchReasoning({ trace }) {
  return (
    <section className="panel reasoning-panel" aria-labelledby="reasoning-heading">
      <div className="panel-header">
        <h2 id="reasoning-heading">Dispatcher reasoning</h2>
        <span className="panel-count">last run</span>
      </div>
      {trace.length === 0 ? (
        <p className="empty-state">Run a dispatch cycle to see the Dispatcher's reasoning here.</p>
      ) : (
        <div className="reasoning-feed">
          {trace.map((line, i) => (
            <div
              key={i}
              className={
                line.startsWith('→') ? 'reasoning-line reasoning-win'
                : line.trim().startsWith('skip') ? 'reasoning-line reasoning-skip'
                : 'reasoning-line'
              }
            >
              {line}
            </div>
          ))}
        </div>
      )}
    </section>
  );
}

export default DispatchReasoning;
