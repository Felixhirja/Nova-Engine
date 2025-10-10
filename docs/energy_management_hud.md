# Energy Management HUD Design

Last updated: 2025-10-09

## Overview

The energy management HUD provides real-time visualization of power distribution across ship subsystems (shields, weapons, thrusters) and enables pilots to adjust allocations dynamically during combat or maneuvering.

## Layout

```text
┌─────────────────────────────────────────────────────────────┐
│                     SHIP STATUS HUD                         │
│                                                             │
│  ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   │
│  │   SHIELDS    │   │   WEAPONS    │   │  THRUSTERS   │   │
│  │  ████████░░  │   │  ██████░░░░  │   │  ██████████  │   │
│  │    85%       │   │    60%       │   │    100%      │   │
│  │  150/180 MJ  │   │   6/10 MW    │   │   10/10 MW   │   │
│  │   +5 MJ/s    │   │              │   │              │   │
│  └──────────────┘   └──────────────┘   └──────────────┘   │
│                                                             │
│  POWER ALLOCATION                         WARNINGS         │
│  Shields:   [████████░░░░] 33%       ⚠ Power Deficit      │
│  Weapons:   [████████░░░░] 33%       ⚠ Shield Recharge    │
│  Thrusters: [████████░░░░] 34%                            │
│                                                             │
│  NET POWER: 26/30 MW     EFFICIENCY: 87%                   │
└─────────────────────────────────────────────────────────────┘
```

## UI Elements

### Shield Status Panel
- **Capacity Bar**: Visual indicator of current/max shield energy (MJ)
- **Percentage**: Numeric display of shield strength (0-100%)
- **Recharge Rate**: Current recharge rate in MJ/s
- **Status Icon**: Color-coded (green=full, yellow=damaged, red=critical, gray=offline)

### Weapon Status Panel
- **Power Bar**: Available weapon power vs requirement
- **Percentage**: Weapon system readiness
- **Ammo/Energy**: Optional ammo count or energy reserves
- **Cooldown Indicator**: Active weapon cooldown timers

### Thruster Status Panel
- **Power Bar**: Thruster power allocation
- **Percentage**: Maneuverability rating based on power
- **Thrust-to-Mass**: Optional performance metric

### Power Allocation Sliders
- **Interactive Bars**: Adjust distribution percentages (keyboard/mouse)
- **Presets**: Quick allocation profiles (balanced, offense, defense, speed)
- **Constraints**: Total must equal 100%, minimum 10% per subsystem

### Warning Indicators
- **Power Deficit**: Flashing red when demand exceeds supply
- **Shield Critical**: Alert when shields drop below 25%
- **Overload Risk**: Warning when approaching overload threshold
- **Recharge Delay**: Countdown timer for shield recharge activation

### Net Power Display
- **Available/Total**: Current power output vs max capacity
- **Efficiency**: Power usage percentage
- **Drain Rate**: MW/s consumption trend

## Interaction Model

### Keyboard Controls
- `[` / `]` – Cycle preset allocations (balanced → offense → defense → speed)
- `Shift + 1/2/3` – Quick divert power to shields/weapons/thrusters
- `P` – Toggle power management overlay detail level

### Mouse Controls
- Click allocation bars to adjust percentages
- Hover for detailed subsystem stats tooltip
- Right-click subsystem panel to toggle on/off

## Visual Feedback

### Color Coding
- **Green**: Optimal performance
- **Yellow**: Reduced efficiency (50-75% power)
- **Orange**: Critical low power (25-50%)
- **Red**: Emergency / offline (<25%)
- **Blue**: Recharging / recovering

### Animations
- Pulse effect on active warnings
- Smooth transitions when adjusting allocations
- Flash effect on damage events
- Progress bar fill animations for recharge

## Presets

| Preset   | Shields | Weapons | Thrusters | Use Case |
|----------|---------|---------|-----------|----------|
| Balanced | 33%     | 33%     | 34%       | Standard operations |
| Offense  | 20%     | 50%     | 30%       | Combat engagement |
| Defense  | 50%     | 25%     | 25%       | Under heavy fire |
| Speed    | 25%     | 20%     | 55%       | Evasive maneuvers, escape |

## Implementation Notes

- HUD renders as OpenGL overlay atop 3D viewport
- Text rendering requires bitmap font or FreeType integration (see `todo_spaceship.txt` text rendering task)
- Update frequency: 10 Hz to balance responsiveness vs. performance
- Data sources: `ShieldSystem`, `WeaponSystem`, `EnergyManagementSystem`
- Optional: Audio cues for warnings (shield down, power critical)

## Dependencies

- Text rendering system (pending)
- Input mapping for allocation controls
- Audio system for alert sounds (optional)

## Future Enhancements

- Subsystem damage visualization
- Per-weapon power toggles
- Shield facet direction control (forward/aft/port/starboard)
- Historical power usage graphs
- AI-assisted auto-balancing mode
