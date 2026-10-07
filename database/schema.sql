-- MedRoute Database Schema
-- Core tables for hospitals, ambulances, emergencies, and dispatch history
--
-- NOTE: This schema was updated to match the core engine design (Phase-II):
--   - A hospital can treat MULTIPLE specializations (matches Hospital::addSpecialization
--     / canTreat() in the C++ engine), so specialization is now a separate junction
--     table instead of a single column.
--   - Emergencies now store the structured emergency_type (e.g. "Cardiac Arrest") that
--     was reported, alongside the severity score it resolved to - matching how
--     SeverityTable works in the engine (type -> severity lookup, not free text).

CREATE TABLE IF NOT EXISTS hospitals (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    location_id INT NOT NULL,       -- corresponds to a node in the road-network graph
    total_beds INT NOT NULL DEFAULT 0,   -- total capacity (fixed per hospital)
    available_beds INT DEFAULT 0         -- currently free (what the engine's Hospital::availableBeds tracks)
);

-- A hospital can treat more than one specialization (e.g. GENERAL + TRAUMA),
-- so this is a many-rows-per-hospital table rather than a single column.
CREATE TABLE IF NOT EXISTS hospital_specializations (
    hospital_id INT NOT NULL,
    specialization VARCHAR(50) NOT NULL,  -- GENERAL, TRAUMA, CARDIAC, PEDIATRIC
    PRIMARY KEY (hospital_id, specialization),
    FOREIGN KEY (hospital_id) REFERENCES hospitals(id)
);

CREATE TABLE IF NOT EXISTS ambulances (
    id INT PRIMARY KEY AUTO_INCREMENT,
    location_id INT NOT NULL,
    status VARCHAR(20) DEFAULT 'FREE'  -- FREE, EN_ROUTE, OCCUPIED
);

CREATE TABLE IF NOT EXISTS emergencies (
    id INT PRIMARY KEY AUTO_INCREMENT,
    location_id INT NOT NULL,
    emergency_type VARCHAR(50) NOT NULL,     -- structured type, e.g. "Cardiac Arrest" (not free text)
    severity INT NOT NULL,                   -- higher = more critical; resolved from emergency_type
    required_specialization VARCHAR(50) NOT NULL,
    reported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS dispatch_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    emergency_id INT NOT NULL,
    ambulance_id INT NOT NULL,
    hospital_id INT NOT NULL,
    pickup_distance DOUBLE,          -- distance from ambulance to patient at dispatch time
    hospital_distance DOUBLE,        -- distance from patient to hospital
    dispatched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (emergency_id) REFERENCES emergencies(id),
    FOREIGN KEY (ambulance_id) REFERENCES ambulances(id),
    FOREIGN KEY (hospital_id) REFERENCES hospitals(id)
);
