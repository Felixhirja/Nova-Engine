# Star Frontier — MVP Scope & Vertical Slice Plan

> A lean, testable path from custom engine prototype to a shippable space sim, sized for a 1–5 dev team in 12 months.

---

## North Star
Deliver a tight, replayable **single‑system** space sim with **flight, combat, mining, trade, and progression** that feels great on keyboard/mouse. Multiplayer and galaxy‑scale ambitions are staged behind a stable core.

**Player promise (vertical slice):**
- Lift off from a station, mine rare ore in a hazardous belt, survive a pirate skirmish, sell cargo, buy a ship upgrade, and feel stronger on the next loop — all in ~25–40 minutes.

**Success criteria (for the slice):**
- 60 FPS on mid‑range PC at 1080p in empty space; 45+ FPS in belt skirmishes.
- New player completes loop without external help in <20 minutes.
- 3+ viable build choices (combat‑leaning, mining‑leaning, balanced).
- Zero blocker crashes in 2‑hour playtests.

---

## Scope Cut (Founder's Edition)
**In now**
- 1 star system (G‑type), 3 planets (1 rocky landable, 1 gas giant w/ moons, 1 ice dwarf), 1 dense asteroid belt.
- 3 stations (trade hub, refinery, pirate outpost), 1 jump gate (decorative, non‑functional for MVP).
- 2 ships: **Fighter** (single‑seat), **Hauler** (light freighter). 1 NPC pirate variant reskin.
- 6 weapon items (2 lasers, 1 autocannon, 2 missile racks, 1 mining laser), 2 shield gens, 2 power plants, 2 thruster sets.
- Newtonian flight with SCM + Cruise; no Quantum Drive yet (replace with in‑system "boost corridors").
- Basic missions: **Courier**, **Mine X and Sell**, **Pirate Bounty**.
- Hazards: radiation pockets in belt, solar micro‑flares that mess with targeting.
- Economy: 5 commodities, simple dynamic pricing with local stock/sink.
- Save/load, single‑player offline; **optional** LAN/solo dedicated server for soak tests.

**Punted to post‑slice**
- Capital ships, boarding, EVA combat, black holes, psionics, governance, permadeath, full jump network, player corps, cosmetics store.

---

## Systems To Build (Vertical Slice)

### Flight & Physics
- **Server‑grade determinism** even in offline mode (fixed tick, e.g., 60 Hz logic / 120 Hz render).
- Thrust model: main, maneuvering, retro; per‑frame force accumulation; heat & power budgets.
- Collision: continuous sweep tests for high‑speed projectiles; broadphase grid or BVH.

### Combat
- Targeting lead indicator with adjustable latency buffer.
- Shields (scalar HP + delay + regen), armor (threshold + damage type modifiers), component damage for **engines, power, weapons** only.
- Countermeasures: flares + basic missile retarget logic.

### Economy & Crafting
- **Sources:** asteroid nodes (3 ores), mission payouts.  
- **Sinks:** fuel, repairs, ammo, station fees, blueprint unlocks.  
- Price = base × (1 + k·(demand−supply)); clamp to [0.5×, 2×].

### Content & World
- Seed‑based system (single seed for MVP) driving orbits, belts, stations, spawn lists.  
- 6 handcrafted POIs layered on proc world: crash site, derelict, refinery queue, pirate nest, research probe, comm buoy.

### UX
- Minimal HUD: throttle/vel, shield/power/heat bars, target widget, radar cone, cargo readout.
- Docking assist, cargo UI, shop UI with compare tooltips.
- Codex entries unlock as you encounter systems/tech.

---

## Networking (Phased)
**Phase 0 (slice):** Offline single‑player running the same netcode loop locally.  
**Phase 1 (post‑slice):** Small dedicated server (12–24 players/instance).

**Model:** Server‑authoritative ECS; client prediction + reconciliation for ships, snapshot interpolation for other entities.  
**Transport:** UDP with reliable/unreliable channels; sequence IDs, delta‑compressed snapshots; interest management via AOI cells (cube grid, ~2–5 km cells).

**Tick/frame:** 30 Hz server tick, 60 Hz client render.  
**Anti‑cheat (later):** sanity checks server‑side; no authoritative client data.

---

## ECS & Data Shapes (Sketch)
```cpp
// Core
struct Transform { vec3 pos; quat rot; };
struct RigidBody { vec3 vel; vec3 angVel; float mass; };
struct NetworkId { uint64 id; };

// Space
struct CelestialBody { float radius; float soi; /* sphere of influence */ };
struct Orbit { uint64 parentId; double a,e,i,raan,argPeri,meanAnom; };

// Ship
struct Ship { float massDry; float cargoKg; };
struct Power { float capMW; float drawMW; };
struct Heat { float gen; float dissipation; float temp; };
struct Shields { float hp; float regen; float delay; };
struct Weapon { enum Type{Laser,Auto,Missile,Mining} type; float dps; float heat; };
struct Hardpoint { uint16 mount; uint64 itemId; };
struct Thruster { vec3 dir; float thrust; float isp; };
struct Inventory { ItemStack stacks[32]; };

// Gameplay
struct Faction { uint16 id; int8 standing; };
struct Market { float stock; float demand; float basePrice; };
struct Mission { uint16 type; uint64 targetId; int state; };
```

**Config example (YAML/JSON):**
```yaml
ship_hauler:
  mass_dry: 22_000
  hardpoints:
    - {mount: 0, accepts: [Laser, Auto]}
    - {mount: 1, accepts: [Missile]}
  power: {cap_mw: 20}
  shields: {hp: 1200, regen: 60, delay: 4}
  thrusters:
    - {dir: [0,0,1], thrust: 180000, isp: 2800}
    - {dir: [0,0,-1], thrust: 40000, isp: 1500}
```

---

## Economy Tuning Cheatsheet
- **Repair fee:** 2–6% of hull MSRP per 10% damage.
- **Fuel:** linear with impulse applied; target 3–5% margin per loop.
- **Ammo:** cap at 10–15% of mission payout for projectile builds.
- **Mining yield:** average loop nets 1.2–1.6× the cost of consumables.
- **Inflation guardrails:** station stock slowly mean‑reverts; daily soft resets for the slice.

---

## Milestones (12 Months)
**M1 (Weeks 1–4)** — Engine/ECS backbone  
- Job system, frame graph, asset hot‑reload, fixed‑tick loop.  
- Headless mode + replay capture.

**M2 (Weeks 5–8)** — Flight feel & docking  
- Newtonian control, HUD basics, station docking, save/load.

**M3 (Weeks 9–12)** — Weapons & shields  
- Lasers, autocannon, missiles, shield model, damage to 3 components.

**M4 (Weeks 13–16)** — Mining & cargo  
- Asteroid scanning, resource nodes, cargo UI, refinery sell loop.

**M5 (Weeks 17–20)** — AI & missions  
- Pirate AI (chase/strafe/retreat), 3 mission templates, hazard events.

**M6 (Weeks 21–24)** — Economy v1 & tuning  
- Market stock/demand, sinks (fuel/repair/ammo/fees), price clamps.

**M7 (Weeks 25–28)** — Content pass  
- 6 POIs, audio temp pass, VFX readability, tutorial scenario.

**M8 (Weeks 29–32)** — Vertical slice polish  
- UX, performance, stability, test harnesses, public demo build.

*(Parallel)* Build & Tools: CI, crash dumps, telemetry, cheats console, perf HUD.

---

## Risk Buy‑Down Spikes
- **Determinism:** record/replay of physics; lockstep test with checksum per tick.
- **Networking skeleton:** local client/server loop with snapshot + prediction in a sandbox map.
- **Save upgrade path:** versioned binary with migrator; fuzz test loads.
- **Interest management:** AOI grid perf test at 1k entities.

---

## Test Plan Highlights
- Unit tests for power/heat budget, price curves, mission rewards.  
- Soak test 2 hours in belt with 20 AI ships; target <0.5% packet loss tolerated without rubber‑banding.
- UX playtests: 10 fresh players, measure time‑to‑first‑sell and death rate.

---

## Open Questions (Answer before M2)
1. **Authoritative units & scale:** meters, kilograms, seconds; max speeds/accelerations.
2. **Physics lib vs. custom:** if custom, which broadphase/narrowphase?  
3. **Tick rates:** 60/30/20 variants and their perf targets.
4. **File formats:** YAML/JSON for authoring → binary packs at build time?  
5. **Modding stance:** data‑driven only for now?  
6. **Accessibility:** color‑blind safe HUD, remappable controls.
7. **Input buffering:** HOTAS later; KB/M first.

---

## Post‑Slice Roadmap (Preview)
- **Co‑op server (12–24 players),** simple party system, shared missions.
- Expand to **M/K‑type systems**, longer‑range travel, quantum drive.
- Add **exploration scanners**, data sell loop, and derelict boarding (non‑combat EVA).
- Begin cosmetics pipeline; no monetization until after slice feedback.

