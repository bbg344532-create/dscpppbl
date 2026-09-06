-- Sample seed data for MedRoute (to be expanded with realistic values)

INSERT INTO hospitals (name, location_id, specialization, available_beds) VALUES
('City General Hospital', 1, 'GENERAL', 20),
('Trauma Care Center', 2, 'TRAUMA', 10);

INSERT INTO ambulances (location_id, status) VALUES
(3, 'FREE'),
(4, 'FREE');

-- TODO: add more realistic sample hospitals/ambulances once real data is decided
