// End-to-end tests: real HTTP server + real SQLite file + the real C++ engine program.
// Build the engine first (see README.md), or point MEDROUTE_ENGINE_PATH at it.
const { test, describe, before, after, beforeEach } = require('node:test');
const assert = require('node:assert/strict');
const fs = require('fs');
const os = require('os');
const path = require('path');
const { createApp } = require('../server');

async function start(options = {}) {
  const dir = fs.mkdtempSync(path.join(os.tmpdir(), 'medroute-'));
  const dbFile = options.dbFile || path.join(dir, 'test.sqlite');
  const { app, close } = await createApp({ dbFile, enginePath: options.enginePath });
  const server = await new Promise((resolve) => { const s = app.listen(0, () => resolve(s)); });
  const base = `http://127.0.0.1:${server.address().port}`;
  const call = async (method, url, body) => {
    const res = await fetch(base + url, { method, headers: body !== undefined ? { 'Content-Type': 'application/json' } : {}, body: body !== undefined ? JSON.stringify(body) : undefined });
    return { status: res.status, body: await res.json() };
  };
  return { call, dbFile, stop: async () => { await new Promise((r) => server.close(r)); close(); } };
}
const sameRoad = (r, a, b) => (r.fromNode === a && r.toNode === b) || (r.fromNode === b && r.toNode === a);
const totalFreeBeds = (state) => state.hospitals.reduce((sum, h) => sum + h.availableBeds, 0);

describe('MedRoute API', () => {
  let s;
  before(async () => { s = await start(); });
  after(async () => { await s.stop(); });
  beforeEach(async () => { await s.call('POST', '/api/reset'); });

  test('health and the starting state', async () => {
    const health = await s.call('GET', '/api/health');
    assert.deepEqual(health.body, { ok: true, database: true, engine: true });
    const { state } = (await s.call('GET', '/api/state')).body;
    assert.equal(state.nodes.length, 30); assert.equal(state.roads.length, 45);
    assert.equal(state.hospitals.length, 9); assert.equal(state.ambulances.length, 9);
    assert.equal(state.emergencyTypes.length, 13); assert.equal(state.scenarios.length, 4);
    assert.deepEqual(state.waiting.map((e) => e.type), ['Cardiac Arrest', 'Severe Bleeding', 'Breathing Difficulty', 'Child Emergency', 'Fracture', 'Minor Injury']);
    assert.equal(state.trips.length, 0);
  });

  test('run dispatch cycle: the engine assigns every free ambulance and the database follows', async () => {
    const before = (await s.call('GET', '/api/state')).body.state;
    const { status, body } = await s.call('POST', '/api/dispatch/run');
    assert.equal(status, 200); assert.equal(body.dispatches.length, 6);
    assert.ok(body.trace.some((l) => l.startsWith('=> AMB-')));
    const st = body.state;
    assert.equal(st.waiting.length, 0); assert.equal(st.trips.length, 6); assert.equal(st.history.length, 6);
    assert.equal(totalFreeBeds(before) - totalFreeBeds(st), 6);
    for (const t of st.trips) assert.equal(st.ambulances.find((a) => a.id === t.ambulanceId).status, 'EN_ROUTE');
    for (const t of st.trips) assert.equal(t.route[t.route.length - 1], st.hospitals.find((h) => h.id === t.hospitalId).node);
  });

  test('report an emergency, then complete a trip: the waiting case is served', async () => {
    await s.call('POST', '/api/dispatch/run');                       // every free ambulance is now busy
    const reported = await s.call('POST', '/api/emergencies', { node: 11, type: 'Burns' });
    assert.equal(reported.status, 201); assert.equal(reported.body.emergencyId, 7);
    assert.equal(reported.body.dispatches.length, 0); assert.deepEqual(reported.body.state.waiting.map((e) => e.id), [7]);
    const first = reported.body.state.trips[0].ambulanceId;
    const done = await s.call('POST', `/api/trips/${first}/complete`);
    assert.equal(done.status, 200); assert.equal(done.body.completed.length, 1);
    assert.equal(done.body.dispatches.length, 1); assert.equal(done.body.dispatches[0].emergencyId, 7);
    assert.equal(done.body.state.waiting.length, 0); assert.equal(done.body.state.history.length, 7);
    assert.ok(done.body.state.history.some((h) => h.completedAt));
  });

  test('traffic event re-plans trips and keeps the base cost', async () => {
    const run = (await s.call('POST', '/api/dispatch/run')).body.state;
    const [a, b] = run.trips[0].route;
    const old = run.roads.find((r) => sameRoad(r, a, b));
    const jam = await s.call('POST', '/api/traffic', { from: a, to: b, cost: old.cost * 6 });
    assert.equal(jam.status, 200); assert.ok(jam.body.reroutes.length >= 1);
    const jammed = jam.body.state.roads.find((r) => sameRoad(r, a, b));
    assert.equal(jammed.cost, old.cost * 6); assert.equal(jammed.baseCost, old.baseCost);
    const clear = await s.call('POST', '/api/traffic', { from: a, to: b, cost: old.baseCost });
    assert.equal(clear.body.state.roads.find((r) => sameRoad(r, a, b)).cost, old.baseCost);
  });

  test('discharge frees a bed, and a freed bed lets a waiting emergency be served', async () => {
    await s.call('POST', '/api/dispatch/run');                       // nothing is waiting and no ambulance is free
    const before = (await s.call('GET', '/api/state')).body.state.hospitals.find((h) => h.id === 1);
    const { status, body } = await s.call('POST', '/api/hospitals/1/discharge');
    assert.equal(status, 200);
    assert.equal(body.state.hospitals.find((h) => h.id === 1).availableBeds, before.availableBeds + 1);
    assert.ok(body.log.some((l) => l.includes('[DISCHARGE] Hospital 1')));
    // With waiting emergencies and free ambulances, a discharge also triggers matching.
    await s.call('POST', '/api/reset');
    const busy = await s.call('POST', '/api/hospitals/1/discharge');
    assert.ok(busy.body.dispatches.length > 0);
  });

  test('scenario: with no cardiac beds, the cardiac case is skipped and another is served', async () => {
    const loaded = await s.call('POST', '/api/scenarios/2/load');
    assert.deepEqual(loaded.body.state.waiting.map((e) => e.type), ['Cardiac Arrest', 'Burns', 'Fracture']);
    assert.equal(loaded.body.state.ambulances.filter((a) => a.status === 'FREE').length, 1);
    const run = await s.call('POST', '/api/dispatch/run');
    assert.equal(run.body.dispatches.length, 1);
    assert.ok(run.body.trace.some((l) => l.startsWith('skip EMG-1') && l.includes('CARDIAC')));
    assert.ok(run.body.state.waiting.some((e) => e.type === 'Cardiac Arrest'));
    assert.equal(run.body.state.history[0].type !== 'Cardiac Arrest', true);
  });

  test('scenario: severity 7 and close beats severity 8 and far', async () => {
    await s.call('POST', '/api/scenarios/3/load');
    const run = await s.call('POST', '/api/dispatch/run');
    assert.equal(run.body.state.history[0].type, 'Breathing Difficulty');
    assert.deepEqual(run.body.state.waiting.map((e) => e.type), ['Accident']);
  });

  test('bad requests get clear errors and change nothing', async () => {
    const before = (await s.call('GET', '/api/state')).body.state;
    const cases = [
      ['POST', '/api/emergencies', { node: 999, type: 'Burns' }, 400, 'UnknownLocationError'],
      ['POST', '/api/emergencies', { node: 11, type: 'Nonsense' }, 400, 'UnknownEmergencyType'],
      ['POST', '/api/emergencies', { node: 'eleven', type: 'Burns' }, 400, 'BadRequest'],
      ['POST', '/api/emergencies', { node: 11 }, 400, 'BadRequest'],
      ['POST', '/api/trips/101/complete', undefined, 409, 'NotEnRoute'],
      ['POST', '/api/trips/999/complete', undefined, 404, 'UnknownIdError'],
      ['POST', '/api/hospitals/999/discharge', undefined, 404, 'UnknownIdError'],
      ['POST', '/api/traffic', { from: 1, to: 999, cost: 5 }, 400, 'UnknownRoadError'],
      ['POST', '/api/traffic', { from: 1, to: 2, cost: -3 }, 400, 'BadRequest'],
      ['POST', '/api/scenarios/999/load', undefined, 404, 'UnknownScenario'],
      ['GET', '/api/nope', undefined, 404, 'NotFound'],
    ];
    for (const [method, url, body, status, type] of cases) {
      const r = await s.call(method, url, body);
      assert.equal(r.status, status, `${method} ${url}`); assert.equal(r.body.ok, false); assert.equal(r.body.error.type, type, `${method} ${url}`);
    }
    assert.deepEqual((await s.call('GET', '/api/state')).body.state, before);
  });

  test('two reports at the same moment get different IDs', async () => {
    await s.call('POST', '/api/dispatch/run');
    const [a, b] = await Promise.all([s.call('POST', '/api/emergencies', { node: 5, type: 'Fracture' }), s.call('POST', '/api/emergencies', { node: 6, type: 'Stroke' })]);
    assert.equal(a.status, 201); assert.equal(b.status, 201);
    assert.notEqual(a.body.emergencyId, b.body.emergencyId);
    assert.equal((await s.call('GET', '/api/state')).body.state.waiting.length, 2);
  });

  test('random session of 50 actions keeps the data consistent', async () => {
    let seed = 2026;
    const rand = (n) => { seed = (seed * 1103515245 + 12345) % 2147483648; return Math.floor(seed / 65536) % n; };
    let st = (await s.call('GET', '/api/state')).body.state;
    for (let step = 0; step < 50; step++) {
      const pick = rand(5); let r;
      if (pick === 0) r = await s.call('POST', '/api/emergencies', { node: 1 + rand(30), type: st.emergencyTypes[rand(st.emergencyTypes.length)].type });
      else if (pick === 1) r = await s.call('POST', '/api/dispatch/run');
      else if (pick === 2) { const en = st.ambulances.filter((a) => a.status === 'EN_ROUTE'); r = en.length ? await s.call('POST', `/api/trips/${en[rand(en.length)].id}/complete`) : await s.call('POST', '/api/dispatch/run'); }
      else if (pick === 3) r = await s.call('POST', `/api/hospitals/${1 + rand(9)}/discharge`);
      else { const road = st.roads[rand(st.roads.length)]; r = await s.call('POST', '/api/traffic', { from: road.fromNode, to: road.toNode, cost: rand(2) ? road.baseCost : road.baseCost * (2 + rand(5)) }); }
      assert.ok([200, 201].includes(r.status), `step ${step} (action ${pick}) failed: ${JSON.stringify(r.body.error)}`);
      st = r.body.state;
      for (const h of st.hospitals) assert.ok(h.availableBeds >= 0 && h.availableBeds <= h.totalBeds, 'beds stay within capacity');
      for (const road of st.roads) assert.ok(road.cost > 0);
      const busy = st.trips.map((t) => t.ambulanceId);
      assert.equal(new Set(busy).size, busy.length, 'no ambulance has two trips');
      for (const id of busy) assert.equal(st.ambulances.find((a) => a.id === id).status, 'EN_ROUTE');
      const waitingIds = new Set(st.waiting.map((e) => e.id));
      for (const t of st.trips) assert.ok(!waitingIds.has(t.emergencyId), 'a dispatched emergency is not also waiting');
    }
  });
});

test('data survives a restart', async () => {
  const first = await start();
  await first.call('POST', '/api/dispatch/run');
  await first.stop();
  const second = await start({ dbFile: first.dbFile });
  const st = (await second.call('GET', '/api/state')).body.state;
  assert.equal(st.trips.length, 6); assert.equal(st.waiting.length, 0);
  await second.stop();
});

test('without the engine, reading works and changing returns a clear 503', async () => {
  const s = await start({ enginePath: path.join(os.tmpdir(), 'no-such-engine') });
  assert.equal((await s.call('GET', '/api/health')).body.engine, false);
  assert.equal((await s.call('GET', '/api/state')).status, 200);
  const r = await s.call('POST', '/api/dispatch/run');
  assert.equal(r.status, 503); assert.equal(r.body.error.type, 'EngineUnavailable');
  assert.equal((await s.call('GET', '/api/state')).body.state.waiting.length, 6);   // nothing changed
  await s.stop();
});
