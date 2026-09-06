# Meet 1 — Checklist & Talking Points

**Goal of this meeting:** Get sign-off on idea, problem statement, features, DS/tech stack (Phase-I) — you've already gotten verbal approval on the hospital routing idea, so this is about presenting it formally and starting the GitHub trail.

---

## What to bring / show

- [ ] **Phase-I PPT** (`medroute_phase1.tex` — already prepared) — walk through: team info → problem statement → objectives → proposed solution → DS/OOP integration table → architecture diagram → tech stack
- [ ] **GitHub repository link** — created and added to PBL portal, with the README (above) already committed
- [ ] **Initial commit(s)** in the repo — even just the README + folder structure counts as a legitimate first commit; don't show an empty repo

---

## Talking points (in case she asks "why this, why like this")

**Why this problem?**
> Emergency response currently relies on manual decisions — which hospital, which ambulance — with no systematic prioritization by severity or real-time availability. We wanted a project with genuine real-world impact, not just a data structure demo.

**Why these data structures specifically?**
> - Priority Queue/Heap — because emergencies must be served by severity, not order of arrival
> - Graph + Dijkstra/A* — because routing between locations is inherently a shortest-path problem
> - Hash Table — for instant lookup of ambulance/hospital status by ID
> - Linked List — for maintaining dispatch history and waitlists in order

**Why this tech stack?**
> The C++ core handles the DS/algorithm-heavy logic (as required by the course), while the web stack (React + Node/Python + MySQL) wraps it into a usable, demoable system — similar to how real backend services separate core logic from the application layer.

**What's simulated vs real?**
> Be upfront: live traffic conditions will be simulated (randomized/time-based congestion) rather than pulled from a real traffic API, since those are often paid/rate-limited. Hospital/ambulance data will be a curated sample dataset for the prototype.

---

## What NOT to over-promise at this stage
- Don't claim any module is "done" yet — Meet 1 is proposal/design only
- Don't commit to a specific UI design yet if frontend hasn't been touched
- Keep the "research idea" section removed, per her last feedback

---

## After this meeting
Once she signs off, the next substantial milestone (Meet 2) is: **finalized architecture diagram, DB schema, and class diagrams** — don't request another meeting until those are actually ready.
