# Solar System Seed Catalog

This catalog provides a curated set of deterministic seeds for the procedural
solar system generator. Each entry has been hand-picked to showcase a specific
scenario so designers and engineers can quickly iterate on gameplay, visual, or
technical validations without rolling random values.

## How to Use

1. Choose a seed from the tables below that matches the scenario you want to
   test.
2. Pass the seed into `SolarSystemGenerator::GenerateSystem(...)` or assign it
   to `GenerationParameters::seed` before calling the generator.
3. Record any tweaks or discoveries directly in this catalog so the rest of the
   team can benefit from the scenario.

```cpp
GenerationParameters params;
params.seed = 42213377; // pick a seed below
Entity star = generator.GenerateSystem(&entityManager, params.seed, params);
```

## Quick Reference Table

| Seed     | Star Type | Primary Feature                | Suggested Focus              |
|----------|-----------|--------------------------------|------------------------------|
| 42213377 | G2V       | Balanced 6-planet layout       | Baseline gameplay + UX       |
| 87651209 | K4V       | Wide habitable zone            | Colony economy balancing     |
| 11004519 | F1V       | Dense inner rocky worlds       | Navigation + thermal visuals |
| 70224481 | M5V       | Gas giant with many moons      | Moon mission prototypes      |
| 99345501 | G9V       | Dual asteroid belts            | Resource loop simulations    |
| 55120073 | A2V       | Hot inner worlds               | Shield/heat system stress    |
| 31888412 | K1V       | Single super-Earth             | Story-driven tutorial flows  |
| 26997740 | G0V       | Twin habitable candidates      | Diplomacy quest mockups      |
| 88411652 | M1V       | Compact red-dwarf system       | Performance benchmarking     |
| 13007731 | F8V       | Station-heavy infrastructure   | Station/mission scripting    |

## Detailed Profiles

### Seed 42213377 — "Kepler's Standard"
- Solar analog with moderate luminosity.
- Six total planets with a mix of rocky inner worlds and two outer gas giants.
- Habitable zone host to one Earth-like candidate and a smaller Mars analogue.
- Ideal for baseline navigation, HUD validation, and UI polish passes.

### Seed 87651209 — "Frontier Roots"
- Orange dwarf with an expansive habitable zone.
- Three mid-sized terrestrial planets spaced widely enough to test long-haul
  flight.
- One outer ice giant and a sparse asteroid belt useful for mining loops.
- Great for tuning FTL or time-acceleration pacing.

### Seed 11004519 — "Sunscorched Chain"
- Bright F-class star with high surface temperature.
- Four close-orbiting rocky planets and a thick inner asteroid torus.
- Tests ship thermal management, radiation hazard UI, and volumetric lighting.

### Seed 70224481 — "Shepherd's Dance"
- Cool red dwarf hosting a dominant gas giant within the habitable zone.
- Gas giant features six procedurally varied moons plus numerous micro-moons.
- Supports mission scripting for moon-hopping campaigns and orbital logistics.

### Seed 99345501 — "Twin Rings"
- Late G-type star with two asteroid belts flanking the main planetary region.
- Rocky inner worlds provide mining choke points; outer giants guard the belts.
- Perfect for economy simulations, pirate ambush testing, and scanner UX.

### Seed 55120073 — "Inferno Reach"
- Hot A-type star radiating intense stellar flux.
- Inner planets feature molten surfaces and scarce atmospheres.
- Use for stress-testing heat shield mechanics and extreme bloom/exposure.

### Seed 31888412 — "Atlas"
- K-type star anchoring a single massive super-Earth with two moons.
- Simplified setup to focus on tutorial scripting or cinematic landings.
- Includes a high-value asteroid cluster beyond the primary orbit.

### Seed 26997740 — "Twin Harbors"
- Sun-like star where two adjacent planets fall inside the habitable band.
- Encourages narrative design around competing colonies or diplomacy scenarios.
- Outer system contains a Trojan station network ideal for mission hubs.

### Seed 88411652 — "Compact Ember"
- Tight-knit M-class system with short orbital periods (<60 days typical).
- High body count (planets, moons, belt chunks) for performance regression
  testing and stress runs on orbital mechanics.

### Seed 13007731 — "Anchorage"
- F-type star with multiple procedurally generated space stations.
- Stations positioned near trade-lanes and Lagrange points with defensive
  escorts.
- Useful for validating docking flow, AI traffic, and quest objective layouts.

## Contribution Guidelines

- When you discover a compelling configuration, add the seed with a short
  description under **Detailed Profiles** and append it to the quick reference
  table.
- Keep descriptions focused on actionable testing contexts instead of pure
  lore.
- If a seed requires special generation parameters (e.g., forcing moons off),
  note the adjustments explicitly so results are reproducible.
