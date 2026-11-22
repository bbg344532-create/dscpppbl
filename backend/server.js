// MedRoute backend. Every request that changes anything runs the real C++ engine to decide.
const express = require('express');
const fs = require('fs');
const path = require('path');
const { Db } = require('./src/db');
const { runEngine, defaultEnginePath } = require('./src/engine');
const { ApiError, fromEngineError } = require('./src/errors');
const { loadState, toEngineText, applyResult, describeState } = require('./src/state');

async function createApp(options = {}) {
  const dbFile = options.dbFile !== undefined ? options.dbFile : (process.env.MEDROUTE_DB || path.join(__dirname, 'data', 'medroute.sqlite'));
  const enginePath = options.enginePath || process.env.MEDROUTE_ENGINE_PATH || defaultEnginePath();
  const db = await Db.open({ file: dbFile, schemaPath: options.schemaPath, seedPath: options.seedPath });

  // Requests that change data run one at a time, so two of them can never act on the same old state.
  let queue = Promise.resolve();
  const exclusively = (fn) => { const run = queue.then(fn); queue = run.catch(() => {}); return run; };

  // Loads the state, asks the engine to apply one action, and saves the result as one all-or-nothing change.
  // 'build' is either the action text, or a function (state) => { action, newEmergency } that runs
  // inside the same exclusive step, so an ID chosen there cannot be taken by another request.
  async function act(build, { check } = {}) {
    return exclusively(async () => {
      const state = loadState(db);
      if (check) check(state);
      const plan = typeof build === 'function' ? build(state) : { action: build };
      const out = await runEngine(toEngineText(state, plan.action), enginePath);
      if (!out.ok) throw fromEngineError(out.error);
      db.transaction(() => applyResult(db, out, plan.newEmergency));
      db.save();
      const { log, trace, dispatches, completed, reroutes, roadChanges } = out;
      const result = { ok: true, action: out.action, log, trace, dispatches, completed, reroutes, roadChanges, state: describeState(db) };
      if (plan.newEmergency) result.emergencyId = plan.newEmergency.id;
      return result;
    });
  }

  const app = express();
  app.use((req, res, next) => {   // open CORS: this is a local prototype
    res.set({ 'Access-Control-Allow-Origin': '*', 'Access-Control-Allow-Headers': 'Content-Type', 'Access-Control-Allow-Methods': 'GET,POST,OPTIONS' });
    if (req.method === 'OPTIONS') return res.sendStatus(204);
    next();
  });
  app.use(express.json());
  const handle = (fn) => (req, res, next) => fn(req, res).catch(next);
  const whole = (value, name) => { if (!Number.isInteger(value)) throw new ApiError(400, 'BadRequest', `${name} must be a whole number`); return value; };

  app.get('/api/health', (req, res) => res.json({ ok: true, database: true, engine: fs.existsSync(enginePath) }));
  app.get('/api/state', handle(async (req, res) => res.json({ ok: true, state: describeState(db) })));
  app.get('/api/history', handle(async (req, res) => res.json({ ok: true, history: describeState(db, Math.min(Number(req.query.limit) || 50, 500)).history })));
  app.get('/api/scenarios', handle(async (req, res) => res.json({ ok: true, scenarios: describeState(db).scenarios })));

  app.post('/api/emergencies', handle(async (req, res) => {
    const body = req.body || {};
    const node = whole(body.node, 'node');
    if (typeof body.type !== 'string') throw new ApiError(400, 'BadRequest', 'type must be text');
    const type = body.type;
    res.status(201).json(await act(() => {
      if (!db.get('SELECT 1 AS x FROM emergency_types WHERE type = ?', [type])) throw new ApiError(400, 'UnknownEmergencyType', `Unknown emergency type '${type}'`);
      const id = db.get('SELECT COALESCE(MAX(id), 0) + 1 AS id FROM emergencies').id;
      return { action: `report ${id} ${node} ${type}`, newEmergency: { id, node, type } };
    }));
  }));

  app.post('/api/dispatch/run', handle(async (req, res) => res.json(await act('run'))));

  app.post('/api/trips/:ambulanceId/complete', handle(async (req, res) => {
    const id = whole(Number(req.params.ambulanceId), 'ambulance ID');
    res.json(await act(`complete ${id}`, { check: (state) => {
      const amb = state.ambulances.find((a) => a.id === id);
      if (!amb) throw new ApiError(404, 'UnknownIdError', `Unknown ambulance ID ${id}`);
      if (amb.status !== 'EN_ROUTE') throw new ApiError(409, 'NotEnRoute', `Ambulance ${id} is not en route`);
    } }));
  }));

  app.post('/api/hospitals/:id/discharge', handle(async (req, res) => {
    res.json(await act(`discharge ${whole(Number(req.params.id), 'hospital ID')}`));
  }));

  app.post('/api/traffic', handle(async (req, res) => {
    const body = req.body || {};
    const from = whole(body.from, 'from'); const to = whole(body.to, 'to');
    if (typeof body.cost !== 'number' || !(body.cost > 0) || !Number.isFinite(body.cost)) throw new ApiError(400, 'BadRequest', 'cost must be a number above zero');
    res.json(await act(`traffic ${from} ${to} ${body.cost}`));
  }));

  app.post('/api/scenarios/:id/load', handle(async (req, res) => {
    const id = whole(Number(req.params.id), 'scenario ID');
    await exclusively(async () => db.reset(id));
    res.json({ ok: true, state: describeState(db) });
  }));
  app.post('/api/reset', handle(async (req, res) => {
    await exclusively(async () => db.reset(null));
    res.json({ ok: true, state: describeState(db) });
  }));

  app.use((req, res) => res.status(404).json({ ok: false, error: { type: 'NotFound', message: `No route ${req.method} ${req.path}` } }));
  app.use((err, req, res, next) => { // eslint-disable-line no-unused-vars
    if (err instanceof ApiError) return res.status(err.status).json({ ok: false, error: { type: err.type, message: err.message } });
    if (err.type === 'entity.parse.failed') return res.status(400).json({ ok: false, error: { type: 'BadRequest', message: 'Request body is not valid JSON' } });
    console.error(err);
    res.status(500).json({ ok: false, error: { type: 'InternalError', message: 'Unexpected server error' } });
  });

  return { app, db, close: () => db.close() };
}

if (require.main === module) {
  const port = Number(process.env.PORT) || 5000;
  createApp().then(({ app }) => app.listen(port, () => console.log(`MedRoute backend listening on port ${port}`)))
    .catch((e) => { console.error('Could not start:', e.message); process.exit(1); });
}

module.exports = { createApp };
