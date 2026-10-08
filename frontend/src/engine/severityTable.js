// Mirrors core-engine/src/SeverityTable.cpp defaults exactly (type -> severity),
// extended with which hospital specialization each type requires — this is the
// "structured input, not free text" design: the caller picks a TYPE, the system
// resolves severity and required specialization from it.
export const EMERGENCY_TYPES = [
  { type: 'Cardiac Arrest', severity: 9, requiredSpecialization: 'CARDIAC' },
  { type: 'Severe Trauma', severity: 8, requiredSpecialization: 'TRAUMA' },
  { type: 'Accident', severity: 8, requiredSpecialization: 'TRAUMA' },
  { type: 'Stroke', severity: 8, requiredSpecialization: 'GENERAL' },
  { type: 'Breathing Difficulty', severity: 7, requiredSpecialization: 'GENERAL' },
  { type: 'Severe Bleeding', severity: 7, requiredSpecialization: 'TRAUMA' },
  { type: 'Child Emergency', severity: 6, requiredSpecialization: 'PEDIATRIC' },
  { type: 'Burns', severity: 6, requiredSpecialization: 'TRAUMA' },
  { type: 'Poisoning', severity: 6, requiredSpecialization: 'GENERAL' },
  { type: 'High Fever', severity: 4, requiredSpecialization: 'GENERAL' },
  { type: 'Fracture', severity: 4, requiredSpecialization: 'GENERAL' },
  { type: 'Minor Injury', severity: 2, requiredSpecialization: 'GENERAL' },
  { type: 'General Checkup', severity: 1, requiredSpecialization: 'GENERAL' },
];

export function lookupType(typeName) {
  return EMERGENCY_TYPES.find((t) => t.type === typeName);
}
