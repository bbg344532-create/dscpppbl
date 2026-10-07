-- Sample seed data for MedRoute
-- Models a small road network (location_id 1-10, matching the core engine's demo
-- graph in main.cpp) with a realistic mix of hospitals, ambulances, and emergencies.

-- =========================================================
-- HOSPITALS
-- =========================================================
INSERT INTO hospitals (id, name, location_id, total_beds, available_beds) VALUES
(1, 'City General Hospital', 8, 20, 12),
(2, 'Metro Cardiac Center', 10, 10, 6),
(3, 'St. Mary''s Children''s Hospital', 4, 12, 8),
(4, 'Riverside Trauma Center', 6, 8, 5),
(5, 'Northside Community Hospital', 2, 18, 15);

-- =========================================================
-- HOSPITAL SPECIALIZATIONS (a hospital can have more than one)
-- =========================================================
INSERT INTO hospital_specializations (hospital_id, specialization) VALUES
(1, 'GENERAL'),
(1, 'TRAUMA'),
(2, 'CARDIAC'),
(3, 'PEDIATRIC'),
(3, 'GENERAL'),
(4, 'TRAUMA'),
(4, 'CARDIAC'),
(5, 'GENERAL');

-- =========================================================
-- AMBULANCES
-- Mix of statuses so the dashboard has something to actually show
-- =========================================================
INSERT INTO ambulances (id, location_id, status) VALUES
(101, 1, 'FREE'),
(102, 9, 'FREE'),
(103, 5, 'EN_ROUTE'),
(104, 7, 'OCCUPIED'),
(105, 3, 'FREE');

-- =========================================================
-- EMERGENCIES
-- Severity values match the engine's SeverityTable (higher = more critical)
-- =========================================================
INSERT INTO emergencies (id, location_id, emergency_type, severity, required_specialization) VALUES
(1, 7, 'Cardiac Arrest', 9, 'CARDIAC'),
(2, 10, 'Fracture', 4, 'GENERAL'),
(3, 6, 'Severe Bleeding', 7, 'TRAUMA'),
(4, 2, 'Breathing Difficulty', 7, 'GENERAL'),
(5, 5, 'Minor Injury', 2, 'GENERAL');

-- =========================================================
-- DISPATCH HISTORY
-- Mirrors the exact outcome of Scenario 1 in the core engine's main.cpp demo,
-- so the database and the live engine demo tell a consistent, cross-checkable story.
-- =========================================================
INSERT INTO dispatch_history (emergency_id, ambulance_id, hospital_id, pickup_distance, hospital_distance) VALUES
(1, 101, 2, 12, 3),
(2, 102, 1, 3, 2),
(3, 101, 1, 8, 6);
