// Runs the C++ engine (core-engine's medroute_service program) once and returns its JSON answer.
const { spawn } = require('child_process');
const fs = require('fs');
const path = require('path');
const { ApiError } = require('./errors');

function defaultEnginePath() {
  const name = process.platform === 'win32' ? 'medroute_service.exe' : 'medroute_service';
  return path.join(__dirname, '..', '..', 'core-engine', name);
}

function runEngine(inputText, enginePath) {
  return new Promise((resolve, reject) => {
    if (!fs.existsSync(enginePath)) {
      return reject(new ApiError(503, 'EngineUnavailable',
        `Engine program not found at ${enginePath}. Build it first (see backend/README.md).`));
    }
    const child = spawn(enginePath, [], { stdio: ['pipe', 'pipe', 'pipe'] });
    let stdout = '';
    let stderr = '';
    const timer = setTimeout(() => {
      child.kill();
      reject(new ApiError(504, 'EngineTimeout', 'The engine did not answer in time.'));
    }, 10000);
    child.stdout.on('data', (d) => { stdout += d; });
    child.stderr.on('data', (d) => { stderr += d; });
    child.on('error', (e) => {
      clearTimeout(timer);
      reject(new ApiError(503, 'EngineUnavailable', `Could not start the engine: ${e.message}`));
    });
    child.on('close', (code) => {
      clearTimeout(timer);
      try {
        resolve(JSON.parse(stdout));
      } catch (e) {
        reject(new ApiError(500, 'EngineError', `Engine gave unreadable output (exit code ${code}): ${(stderr || stdout).slice(0, 200)}`));
      }
    });
    child.stdin.on('error', () => {});
    child.stdin.end(inputText);
  });
}

module.exports = { runEngine, defaultEnginePath };
