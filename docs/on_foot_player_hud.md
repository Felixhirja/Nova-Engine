# On-Foot Player HUD Design

Last updated: 2025-01-06

## Overview

The on-foot player HUD provides situational awareness and quick access to suit functions during first-person planetary exploration and station boarding sequences. The layout prioritizes legibility against diverse lighting environments while minimizing screen clutter.

## Layout

```text
┌──────────────────────────────────────────────────────────────────────┐
│                       ON-FOOT EXPLORATION HUD                        │
│                                                                      │
│  ┌──────────────┐                     ┌──────────────┐               │
│  │ HEALTH  92%  │                     │ SHIELD  65%  │               │
│  │ ████████░░░  │                     │ █████░░░░░░  │               │
│  └──────────────┘                     └──────────────┘               │
│                                                                      │
│  STAMINA [████████░░░░░░░░] 60%             TEMP  18°C (Stable)       │
│                                                                      │
│  Ammo: 42 / 120 (Pulse Rifle)            Grenades: 2 (EMP)           │
│                                                                      │
│  ┌──────────────────────────────────────────────────────────────┐    │
│  │ Mission: Locate data shard                                   │    │
│  │ • Reach research wing                                         │    │
│  │ • Bypass security drones (Optional)                           │    │
│  │ • Extract shard and exfiltrate                                │    │
│  └──────────────────────────────────────────────────────────────┘    │
│                                                                      │
│  Compass: 312° NW            Distance to objective: 145 m             │
│                                                                      │
│  Interact [E]    Flashlight [F]    Scanner [Q]    Ability [Mouse4]    │
└──────────────────────────────────────────────────────────────────────┘
```

## UI Elements

### Vital Status Panels
- **Health Panel**: Displays current suit integrity as percentage and bar. Flashes red and pulses when below 25%.
- **Shield Panel**: Shows personal energy shield charge. Grays out when depleted and animates recharge progress.
- **Temperature Readout**: Indicates ambient exposure; changes color based on thresholds (blue=cold risk, amber=heat risk).

### Mobility & Endurance
- **Stamina Bar**: Horizontal meter draining during sprinting, climbing, and melee. Regenerates when stationary. Color shifts from green → yellow → red as stamina falls.
- **Status Effects Icons**: Optional small icons above the bar for slowed, concussed, or boosted states with tooltip descriptions on hover.

### Combat Readiness
- **Ammo Counter**: Primary weapon magazine count and reserve. Flashes when low (<15% of reserve) and animates on reload.
- **Grenade Inventory**: Icon + count for current throwable type; scroll wheel or number keys to cycle when multiple types available.
- **Ability Cooldown Widget**: Radial meter tied to suit abilities (dash, cloak). Supports multiple simultaneous abilities stacked in a row.

### Navigation & Objectives
- **Compass Ribbon**: Horizontal strip at top-center showing cardinal directions and icons for nearby waypoints, teammates, and threats.
- **Objective Tracker**: Mission panel listing primary and optional tasks. Collapsible via `Tab` to reduce clutter.
- **Distance Indicator**: Numeric display to active objective with color-coded proximity (green >100 m, yellow 30-100 m, red <30 m).

### Interaction Prompts
- **Context Prompt**: Dynamic `Interact [E]` message near reticle when actionable objects within range. Auto-fades after interaction.
- **Reticle**: Minimal crosshair that expands during recoil and contracts while aiming down sights.
- **Scanner Overlay**: When active, dims background and highlights points of interest; timer shows remaining scan duration.

## Interaction Model

### Keyboard & Mouse
- `WASD` – Movement
- `Shift` – Sprint (drains stamina)
- `Ctrl` – Crouch / slide depending on momentum
- `Space` – Jump / mantle
- `E` – Interact
- `F` – Toggle flashlight
- `Q` – Activate scanner
- `Mouse4/Mouse5` – Trigger suit abilities
- `Tab` – Expand / collapse objective panel
- `Mouse Scroll` – Cycle grenade types

### Gamepad
- `Left Stick` – Movement
- `Right Stick` – Look / aim
- `A` / `Cross` – Jump / mantle
- `B` / `Circle` – Crouch / slide
- `X` / `Square` – Interact
- `Y` / `Triangle` – Cycle grenades
- `RB` – Scanner
- `LB` – Ability 1
- `RT` – Primary fire
- `LT` – Aim down sights (tightens reticle)
- `D-Pad Up` – Toggle flashlight
- `D-Pad Down` – Collapse objective panel

## Visual Feedback

### Color Coding
- **Green**: Stable condition / ability ready
- **Yellow**: Moderate risk (stamina <60%, shields <40%)
- **Red**: Critical status (health <25%, stamina <15%)
- **Cyan**: Active scanner mode
- **Purple**: Special ability ready / empowered state

### Animations & Audio
- Health and shield panels shake subtly on receiving damage.
- Objective updates slide in from left with confirmation chime.
- Stamina bar refills with smooth gradient sweep.
- Ability cooldown widgets rotate clockwise during recharge.
- Warning tones escalate when multiple critical alerts overlap.

## Accessibility Considerations
- Colorblind-safe palette presets (Protanopia, Deuteranopia, Tritanopia) for vital indicators.
- Adjustable HUD opacity and scale via settings menu.
- Optional textual callouts for audio cues (e.g., "Shield Critical").
- Toggleable minimalist mode removing non-essential panels for expert players.

## Implementation Notes
- Renders as layered 2D UI pass after weapon model but before fullscreen effects.
- Uses existing text renderer shared with energy management HUD for consistency.
- Data sources: `OnFootHealthComponent`, `PersonalShieldComponent`, `InventoryComponent`, `MissionSystem`, `NavigationSystem`.
- Requires event hooks for stamina drain/regeneration and context prompt detection.
- Ensure UI scales to 21:9 and 16:10 aspect ratios without overlap.

## Future Enhancements
- Squad status mini-panels showing teammate vitals.
- Environmental hazard alerts (radiation, toxins) integrated near temperature readout.
- Mission voice-over transcription overlay for accessibility.
- Augmented reality scan highlighting weak points on enemies.
