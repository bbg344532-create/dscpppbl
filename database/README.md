# MedRoute database

| File | Purpose |
|---|---|
| `schema.sql` | The 12 tables (MySQL-style SQL; the backend converts `AUTO_INCREMENT` for SQLite) |
| `seed_data.sql` | The demo city: 30 nodes, 45 roads, 9 hospitals, 9 ambulances, 13 emergency types, 6 waiting emergencies, 4 scenarios. **Generated: do not edit by hand** |
| `generate_data.py` | Regenerates `seed_data.sql` and `city.json` (deterministic: same output every run) |
| `check_data.py` | Loads schema + seed into SQLite and checks them (graph connected, foreign keys, bed limits, scenario outcomes) |
| `city.json` | The same data as JSON; used by the service cross-check |

```
python generate_data.py
python check_data.py
```

All data is invented demo data: no real places, hospitals, traffic or ambulances.
