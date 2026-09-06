-- MedRoute Database Schema
-- Core tables for hospitals, ambulances, emergencies, and dispatch history

CREATE TABLE IF NOT EXISTS hospitals (
    id INT PRIMARY KEY AUTO_INCREMENT,
    name VARCHAR(100) NOT NULL,
    location_id INT NOT NULL,       -- corresponds to a node in the road-network graph
    specialization VARCHAR(50),     -- e.g., GENERAL, TRAUMA, CARDIAC, PEDIATRIC
    available_beds INT DEFAULT 0
);

CREATE TABLE IF NOT EXISTS ambulances (
    id INT PRIMARY KEY AUTO_INCREMENT,
    location_id INT NOT NULL,
    status VARCHAR(20) DEFAULT 'FREE'  -- FREE, EN_ROUTE, OCCUPIED
);

CREATE TABLE IF NOT EXISTS emergencies (
    id INT PRIMARY KEY AUTO_INCREMENT,
    location_id INT NOT NULL,
    severity INT NOT NULL,             -- lower = more critical
    required_specialization VARCHAR(50),
    reported_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS dispatch_history (
    id INT PRIMARY KEY AUTO_INCREMENT,
    emergency_id INT NOT NULL,
    ambulance_id INT NOT NULL,
    hospital_id INT NOT NULL,
    dispatched_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (emergency_id) REFERENCES emergencies(id),
    FOREIGN KEY (ambulance_id) REFERENCES ambulances(id),
    FOREIGN KEY (hospital_id) REFERENCES hospitals(id)
);
