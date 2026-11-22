// SQLite database (sql.js: SQLite compiled to WebAssembly, so nothing native to install).
// The whole database lives in memory and is written to one file after every change.
const fs = require('fs');
const path = require('path');
const initSqlJs = require('sql.js');
const { ApiError } = require('./errors');

const DB_FOLDER = path.join(__dirname, '..', '..', 'database');

// schema.sql is written in MySQL-style syntax; this makes it valid for SQLite.
function sqliteScript(file) {
  return fs.readFileSync(file, 'utf8').replace(/INT PRIMARY KEY AUTO_INCREMENT/g, 'INTEGER PRIMARY KEY AUTOINCREMENT');
}

class Db {
  static async open({ file, schemaPath, seedPath }) {
    const SQL = await initSqlJs();
    const db = new Db(SQL, file, schemaPath || path.join(DB_FOLDER, 'schema.sql'), seedPath || path.join(DB_FOLDER, 'seed_data.sql'));
    if (file && fs.existsSync(file)) {
      db.raw = new SQL.Database(fs.readFileSync(file));
      db.raw.run('PRAGMA foreign_keys = ON');
    } else {
      db.buildFresh(null);
      db.save();
    }
    return db;
  }

  constructor(SQL, file, schemaPath, seedPath) {
    this.SQL = SQL;
    this.file = file;
    this.schemaPath = schemaPath;
    this.seedPath = seedPath;
    this.raw = null;
  }

  // Builds a brand-new database from schema + seed, optionally with a demo scenario applied on top.
  buildFresh(scenarioId) {
    const raw = new this.SQL.Database();
    raw.run('PRAGMA foreign_keys = ON');
    raw.exec(sqliteScript(this.schemaPath));
    raw.exec(sqliteScript(this.seedPath));
    if (scenarioId !== null && scenarioId !== undefined) {
      const found = raw.exec('SELECT COUNT(*) FROM scenarios WHERE id = ' + Number(scenarioId))[0].values[0][0];
      if (!found) { raw.close(); throw new ApiError(404, 'UnknownScenario', `No scenario with ID ${scenarioId}`); }
      const id = Number(scenarioId);
      raw.run('UPDATE hospitals SET available_beds = (SELECT available_beds FROM scenario_hospital_beds s WHERE s.scenario_id = ? AND s.hospital_id = hospitals.id) WHERE id IN (SELECT hospital_id FROM scenario_hospital_beds WHERE scenario_id = ?)', [id, id]);
      raw.run('UPDATE ambulances SET location_id = (SELECT location_id FROM scenario_ambulances s WHERE s.scenario_id = ? AND s.ambulance_id = ambulances.id), status = (SELECT status FROM scenario_ambulances s WHERE s.scenario_id = ? AND s.ambulance_id = ambulances.id) WHERE id IN (SELECT ambulance_id FROM scenario_ambulances WHERE scenario_id = ?)', [id, id, id]);
      raw.run('DELETE FROM emergencies');
      raw.run("DELETE FROM sqlite_sequence WHERE name = 'emergencies'");
      raw.run('INSERT INTO emergencies (location_id, emergency_type) SELECT location_id, emergency_type FROM scenario_emergencies WHERE scenario_id = ? ORDER BY report_order', [id]);
    }
    if (this.raw) this.raw.close();
    this.raw = raw;
  }

  reset(scenarioId) {
    this.buildFresh(scenarioId);
    this.save();
  }

  all(sql, params = []) {
    const stmt = this.raw.prepare(sql);
    try {
      stmt.bind(params);
      const rows = [];
      while (stmt.step()) rows.push(stmt.getAsObject());
      return rows;
    } finally {
      stmt.free();
    }
  }

  get(sql, params = []) {
    return this.all(sql, params)[0];
  }

  run(sql, params = []) {
    this.raw.run(sql, params);
  }

  // Runs fn as one all-or-nothing change.
  transaction(fn) {
    this.raw.run('BEGIN');
    try {
      const result = fn();
      this.raw.run('COMMIT');
      return result;
    } catch (e) {
      this.raw.run('ROLLBACK');
      throw e;
    }
  }

  save() {
    if (!this.file) return;
    fs.mkdirSync(path.dirname(this.file), { recursive: true });
    const tmp = this.file + '.tmp';
    fs.writeFileSync(tmp, Buffer.from(this.raw.export()));
    fs.renameSync(tmp, this.file);
    this.raw.run('PRAGMA foreign_keys = ON'); // export() resets pragmas
  }

  close() {
    if (this.raw) this.raw.close();
    this.raw = null;
  }
}

module.exports = { Db };
