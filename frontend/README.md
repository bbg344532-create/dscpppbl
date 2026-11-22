# MedRoute frontend

The operator dashboard (React + Vite). It holds no data and makes no decisions: it reads everything from the
backend and sends the operator's actions to it. Every dispatch decision is made by the C++ engine behind the backend.

## Run (needs Node.js 18 or newer)

Start the backend first (see `../backend/README.md`), then:

```
npm install
npm run dev
```

Open the address it prints (usually http://localhost:5173).

The backend address defaults to `http://localhost:5000`. To change it, copy `.env.example` to `.env` and edit
`VITE_API_URL`.

## What you can do

| Panel | Action |
|---|---|
| Report emergency | Pick a node and an emergency type (severity and needed specialization fill in from the database), then Add to queue |
| Run dispatch cycle | The engine matches free ambulances to waiting emergencies; the reasoning panel shows its scores and skipped cases |
| Ambulances | Complete a trip: the ambulance is freed at the hospital and waiting emergencies are matched again |
| Hospitals | Discharge one patient (frees a bed) |
| Traffic | Pick a road (or click one on the map), set a new cost, Apply. Trips under way are re-planned. Clear removes a jam |
| Demo scenarios | Load a ready-made situation, or reset all data |

The map is drawn from the database: rings with a plus are hospitals, squares are ambulances, circles are waiting
emergencies, diamonds are ambulance depots, orange roads are jammed, the thick teal line is the latest dispatch route.

If the backend is not running the page says so and offers a retry; if it stops while the page is open, the last data stays
on screen with a warning.

## Optional browser test

`e2e/e2e.cjs` drives the real page in a headless browser against a real backend (it starts the backend itself). It needs
the engine program built and Playwright installed (`npm install --no-save playwright`, then `npx playwright install chromium`):

```
npm run build
MEDROUTE_ENGINE_PATH=../core-engine/medroute_service node e2e/e2e.cjs
```
