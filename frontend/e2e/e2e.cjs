// Browser test: real backend + real engine + the built page, driven with headless Chromium.
// Run `npm run build` first (with the same VITE_API_URL), then: node e2e/e2e.cjs
const { spawn } = require('child_process');
const fs = require('fs');
const http = require('http');
const os = require('os');
const path = require('path');
const { chromium } = require('playwright');

const API = process.env.VITE_API_URL || 'http://localhost:5000';
const PORT = new URL(API).port || '5000';
const SHOTS = process.env.E2E_SHOTS || '';
const dist = path.join(__dirname, '..', 'dist');
const mime = { '.html': 'text/html', '.js': 'text/javascript', '.css': 'text/css' };
let passed = 0; const failed = [];
const check = (cond, msg) => { if (cond) passed += 1; else { failed.push(msg); console.log('FAIL  ' + msg); } };

(async () => {
  const site = http.createServer((req, res) => {
    const file = path.join(dist, req.url === '/' ? 'index.html' : req.url.split('?')[0]);
    if (!file.startsWith(dist) || !fs.existsSync(file)) { res.writeHead(404); return res.end(); }
    res.writeHead(200, { 'Content-Type': mime[path.extname(file)] || 'application/octet-stream' });
    fs.createReadStream(file).pipe(res);
  });
  await new Promise((r) => site.listen(0, r));
  const siteUrl = `http://127.0.0.1:${site.address().port}`;

  const dbFile = path.join(fs.mkdtempSync(path.join(os.tmpdir(), 'medroute-e2e-')), 'e2e.sqlite');
  const backend = spawn('node', [path.join(__dirname, '..', '..', 'backend', 'server.js')], { env: { ...process.env, PORT, MEDROUTE_DB: dbFile }, stdio: 'ignore' });
  for (let i = 0; i < 50; i += 1) { try { await fetch(API + '/api/health'); break; } catch { await new Promise((r) => setTimeout(r, 100)); } }

  const browser = await chromium.launch();
  const page = await browser.newPage({ viewport: { width: 1500, height: 1000 } });
  const errors = [];
  page.on('pageerror', (e) => errors.push(e.message));
  const count = (sel) => page.locator(sel).count();
  const until = (fn, arg) => page.waitForFunction(fn, arg, { timeout: 10000 });
  const waitingIs = (n) => until((k) => document.querySelectorAll('[data-testid="waiting-item"]').length === k, n);
  const text = (sel) => page.locator(sel).innerText();
  const shot = async (name) => { if (SHOTS) await page.screenshot({ path: path.join(SHOTS, name + '.png'), fullPage: true }); };

  await page.goto(siteUrl);
  await waitingIs(6);
  check((await text('[data-testid="connection"]')).includes('Connected to backend'), 'header shows the backend as connected');
  check((await count('.map-node')) + (await count('.map-depot')) === 30, 'map draws all 30 nodes');
  check((await count('.map-hospital-circle')) === 9, 'map draws 9 hospitals');
  check((await count('[data-testid="ambulance-item"]')) === 9 && (await count('[data-testid="hospital-item"]')) === 9, 'lists show 9 ambulances and 9 hospitals');
  await shot('1-start');

  await page.getByRole('button', { name: 'Run dispatch cycle' }).click();
  await waitingIs(0);
  check((await count('[data-testid="log-entry"]')) === 6, 'run: six dispatches appear in the log');
  check((await text('[data-testid="reasoning"]')).includes('=> AMB-'), 'run: reasoning panel shows the winners');
  check((await count('.map-edge-highlight')) > 0, 'run: the new routes are highlighted on the map');
  await shot('2-after-run');

  await page.selectOption('select[aria-label="Location"]', '11');
  await page.selectOption('select[aria-label="Emergency type"]', 'Burns');
  check((await text('.resolved-preview')).includes('severity 6'), 'form: severity and specialization preview from the type table');
  await page.getByRole('button', { name: 'Add to queue' }).click();
  await waitingIs(1);
  check((await text('[data-testid="waiting-item"]')).includes('Burns'), 'report: the new emergency is waiting');

  await page.getByRole('button', { name: 'Complete' }).first().click();
  await waitingIs(0);
  check((await count('[data-testid="log-entry"]')) === 7, 'complete: the waiting emergency is served (7 log entries)');

  await page.selectOption('select[aria-label="Road"]', { index: 1 });
  await page.locator('input[aria-label="New cost"]').fill('40');
  await page.getByRole('button', { name: 'Apply' }).click();
  await until(() => document.querySelectorAll('[data-testid="jam-item"]').length === 1);
  check((await count('.map-edge-jam')) === 1, 'traffic: the jammed road is orange on the map');
  check((await text('[data-testid="reasoning"]')).includes('[TRAFFIC]'), 'traffic: the engine log mentions the change');
  await shot('3-traffic');
  await page.locator('[data-testid="jam-item"] button').click();
  await until(() => document.querySelectorAll('[data-testid="jam-item"]').length === 0);
  check((await count('.map-edge-jam')) === 0, 'traffic: clearing the jam restores the road');
  await page.locator('[data-testid="road"]').nth(5).click({ force: true });
  check((await page.locator('select[aria-label="Road"]').inputValue()) !== '', 'clicking a road on the map selects it');

  await page.getByRole('button', { name: /Discharge a patient at City General/ }).click();
  await until(() => document.querySelector('[data-testid="reasoning"]') && document.querySelector('[data-testid="reasoning"]').innerText.includes('[DISCHARGE] Hospital 1'));
  check(true, 'discharge: the engine log shows the freed bed');

  await page.selectOption('select[aria-label="Scenario"]', { label: 'Cardiac capacity exhausted' });
  await page.getByRole('button', { name: 'Load scenario' }).click();
  await waitingIs(3);
  await page.getByRole('button', { name: 'Run dispatch cycle' }).click();
  await waitingIs(2);
  const reasoning = await text('[data-testid="reasoning"]');
  check(reasoning.includes('skip EMG-1') && reasoning.includes('CARDIAC'), 'scenario: the cardiac case is skipped and the reason is shown');
  await shot('4-scenario');

  await page.getByRole('button', { name: 'Reset all data' }).click();
  await waitingIs(6);
  check((await count('[data-testid="log-entry"]')) === 0, 'reset: the history is empty again');

  backend.kill();
  await new Promise((r) => setTimeout(r, 500));
  await page.getByRole('button', { name: 'Run dispatch cycle' }).click();
  await until(() => document.querySelector('[data-testid="notice"]'));
  check((await text('[data-testid="notice"]')).includes('Cannot reach the backend'), 'offline: a clear error is shown');
  check((await count('[data-testid="waiting-item"]')) === 6, 'offline: the last data stays on screen');
  await shot('5-offline');

  check(errors.length === 0, 'no JavaScript errors in the page' + (errors.length ? ': ' + errors[0] : ''));
  await browser.close(); site.close(); backend.kill();
  console.log(`\n${passed} checks passed, ${failed.length} failed`);
  process.exit(failed.length ? 1 : 0);
})().catch((e) => { console.error('E2E crashed:', e.message); process.exit(1); });
