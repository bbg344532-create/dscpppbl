-- MedRoute Database Schema (Phase-III, Session 1)
-- Single source of truth for the road network, emergency types, hospitals,
-- ambulances and demo scenarios. Written in MySQL-style syntax; every
-- statement also runs on SQLite (AUTO_INCREMENT is swapped in the test script).
--
-- New in Phase-III: nodes + roads (the graph used to be hardcoded in main.cpp
-- and graph.js), emergency_types (severity table used to be hardcoded),
-- and scenario tables (ready-made demo situations).

CREATE TABLE IF NOT EXISTS nodes (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    x INT NOT NULL,                              -- drawing position on the map
    y INT NOT NULL,
    node_type VARCHAR(20) NOT NULL DEFAULT 'AREA',   -- AREA, HOSPITAL, DEPOT
    CHECK (node_type IN ('AREA', 'HOSPITAL', 'DEPOT'))
);

-- Roads are two-way; each road is stored once with from_node < to_node.
CREATE TABLE IF NOT EXISTS roads (
    id INT PRIMARY KEY AUTO_INCREMENT,
    from_node INT NOT NULL,
    to_node INT NOT NULL,
    weight DOUBLE NOT NULL,                      -- current travel cost, always > 0 (changes with traffic)
    base_weight DOUBLE NOT NULL,                 -- normal cost, kept so a traffic jam can be cleared
    UNIQUE (from_node, to_node),
    CHECK (weight > 0),
    CHECK (base_weight > 0),
    CHECK (from_node < to_node),
    FOREIGN KEY (from_node) REFERENCES nodes(id),
    FOREIGN KEY (to_node) REFERENCES nodes(id)
);

-- Structured emergency types: the caller picks a type, never free text.
CREATE TABLE IF NOT EXISTS emergency_types (
    type VARCHAR(50) PRIMARY KEY,
    severity INT NOT NULL,                       -- higher = more critical
    required_specialization VARCHAR(50) NOT NULL -- GENERAL, TRAUMA, CARDIAC, PEDIATRIC
);

CREATE TABLE IF NOT EXISTS hospitals (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    location_id INT NOT NULL,
    total_beds INT NOT NULL DEFAULT 0,
    available_beds INT NOT NULL DEFAULT 0,
    CHECK (available_beds >= 0 AND available_beds <= total_beds),
    FOREIGN KEY (location_id) REFERENCES nodes(id)
);

CREATE TABLE IF NOT EXISTS hospital_specializations (
    hospital_id INT NOT NULL,
    specialization VARCHAR(50) NOT NULL,
    PRIMARY KEY (hospital_id, specialization),
    FOREIGN KEY (hospital_id) REFERENCES hospitals(id)
);

CREATE TABLE IF NOT EXISTS ambulances (
    id INT PRIMARY KEY AUTO_INCREMENT,
    location_id INT NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'FREE',
    CHECK (status IN ('FREE', 'EN_ROUTE', 'OCCUPIED')),
    FOREIGN KEY (location_id) REFERENCES nodes(id)
);

CREATE TABLE IF NOT EXISTS emergencies (
    id INT PRIMARY KEY AUTO_INCREMENT,
    location_id INT NOT NULL,
    emergency_type VARCHAR(50) NOT NULL,
    status VARCHAR(20) NOT NULL DEFAULT 'WAITING',   -- WAITING, DISPATCHED, COMPLETED
    reported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    CHECK (status IN ('WAITING', 'DISPATCHED', 'COMPLETED')),
    FOREIGN KEY (location_id) REFERENCES nodes(id),
    FOREIGN KEY (emergency_type) REFERENCES emergency_types(type)
);

CREATE TABLE IF NOT EXISTS dispatch_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    emergency_id INT NOT NULL,
    ambulance_id INT NOT NULL,
    hospital_id INT NOT NULL,
    pickup_distance DOUBLE,
    hospital_distance DOUBLE,
    route TEXT,                                  -- JSON list of node IDs, ambulance start to hospital
    cost DOUBLE,                                 -- total route cost
    dispatched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    completed_at TIMESTAMP NULL,                 -- NULL while the trip is still in progress
    FOREIGN KEY (emergency_id) REFERENCES emergencies(id),
    FOREIGN KEY (ambulance_id) REFERENCES ambulances(id),
    FOREIGN KEY (hospital_id) REFERENCES hospitals(id)
);

-- Ready-made demo situations. Loading a scenario means: take the base
-- hospitals/ambulances, apply the overrides below, and use the scenario's
-- own emergency list as the queue (instead of the default emergencies table).
CREATE TABLE IF NOT EXISTS scenarios (
    id INT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    description VARCHAR(500) NOT NULL
);

CREATE TABLE IF NOT EXISTS scenario_emergencies (
    id INT PRIMARY KEY AUTO_INCREMENT,
    scenario_id INT NOT NULL,
    location_id INT NOT NULL,
    emergency_type VARCHAR(50) NOT NULL,
    report_order INT NOT NULL,
    FOREIGN KEY (scenario_id) REFERENCES scenarios(id),
    FOREIGN KEY (location_id) REFERENCES nodes(id),
    FOREIGN KEY (emergency_type) REFERENCES emergency_types(type)
);

CREATE TABLE IF NOT EXISTS scenario_hospital_beds (
    scenario_id INT NOT NULL,
    hospital_id INT NOT NULL,
    available_beds INT NOT NULL,
    PRIMARY KEY (scenario_id, hospital_id),
    FOREIGN KEY (scenario_id) REFERENCES scenarios(id),
    FOREIGN KEY (hospital_id) REFERENCES hospitals(id)
);

CREATE TABLE IF NOT EXISTS scenario_ambulances (
    scenario_id INT NOT NULL,
    ambulance_id INT NOT NULL,
    location_id INT NOT NULL,
    status VARCHAR(20) NOT NULL,
    PRIMARY KEY (scenario_id, ambulance_id),
    CHECK (status IN ('FREE', 'EN_ROUTE', 'OCCUPIED')),
    FOREIGN KEY (scenario_id) REFERENCES scenarios(id),
    FOREIGN KEY (ambulance_id) REFERENCES ambulances(id),
    FOREIGN KEY (location_id) REFERENCES nodes(id)
);
