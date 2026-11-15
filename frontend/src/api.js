// Every call to the MedRoute backend goes through here.
const BASE = (import.meta.env.VITE_API_URL || 'http://localhost:5000').replace(/\/$/, '');

export class ApiClientError extends Error {
  constructor(message, { status = 0, type = 'NetworkError' } = {}) {
    super(message);
    this.status = status;
    this.type = type;
  }
}

async function request(method, path, body) {
  let res;
  try {
    res = await fetch(BASE + path, {
      method,
      headers: body !== undefined ? { 'Content-Type': 'application/json' } : undefined,
      body: body !== undefined ? JSON.stringify(body) : undefined,
    });
  } catch {
    throw new ApiClientError(`Cannot reach the backend at ${BASE}. Is it running?`);
  }
  let data = null;
  try { data = await res.json(); } catch { /* not JSON */ }
  if (!res.ok || !data || data.ok === false) {
    const e = data && data.error;
    throw new ApiClientError(e ? e.message : `Request failed (${res.status})`, { status: res.status, type: e ? e.type : 'HttpError' });
  }
  return data;
}

export const api = {
  baseUrl: BASE,
  health: () => request('GET', '/api/health'),
  getState: () => request('GET', '/api/state').then((d) => d.state),
  reportEmergency: (node, type) => request('POST', '/api/emergencies', { node, type }),
  runDispatch: () => request('POST', '/api/dispatch/run'),
  completeTrip: (ambulanceId) => request('POST', `/api/trips/${ambulanceId}/complete`),
  discharge: (hospitalId) => request('POST', `/api/hospitals/${hospitalId}/discharge`),
  setTraffic: (from, to, cost) => request('POST', '/api/traffic', { from, to, cost }),
  loadScenario: (id) => request('POST', `/api/scenarios/${id}/load`),
  reset: () => request('POST', '/api/reset'),
};
