# Player Configuration Guide

## Overview
The player character is now fully configurable through JSON files without touching any code! This makes it easy for designers and beginners to customize the player behavior.

## Configuration File
Edit: `assets/config/player_config.json`

## Settings Explained

### Spawn Position
```json
"spawn": {
  "position": {
    "x": 0.0,   // Left (-) / Right (+) starting position
    "y": 0.0,   // Down (-) / Up (+) starting position  
    "z": 0.0    // Backward (-) / Forward (+) starting position
  }
}
```

### Movement Settings
```json
"movement": {
  "forward_speed": 5.0,     // How fast W/S keys move
  "backward_speed": 5.0,    // Maximum backward speed
  "strafe_speed": 5.0,      // How fast A/D keys move
  "up_down_speed": 5.0,     // How fast Space/C keys move
  "acceleration": 4.0,      // How quickly speed builds up
  "deceleration": 4.0,      // How quickly player stops
  "friction": 0.0           // Air resistance (0.0 = none, 1.0 = max)
}
```

### Physics Settings
```json
"physics": {
  "enable_gravity": false,    // true = player falls, false = free movement
  "gravity_strength": -9.8,   // How strong gravity is (negative = down)
  "max_ascent_speed": 10.0,   // Maximum upward speed
  "max_descent_speed": -20.0  // Maximum downward speed
}
```

### Visual Settings
```json
"visual": {
  "color": {
    "r": 0.2,  // Red component (0.0 to 1.0)
    "g": 0.8,  // Green component (0.0 to 1.0)
    "b": 1.0   // Blue component (0.0 to 1.0)
  },
  "scale": 0.5,    // Size of player (1.0 = normal, 0.5 = half size)
  "mesh_id": 0     // 3D model (0 = default cube)
}
```

### Camera Settings
```json
"camera": {
  "priority": 100,   // Camera priority (higher = takes control)
  "is_active": true  // Whether camera follows this player
}
```

## Quick Examples

### Make player start at different position:
```json
"spawn": {
  "position": { "x": 10.0, "y": 5.0, "z": -20.0 }
}
```

### Make player faster:
```json
"movement": {
  "forward_speed": 10.0,
  "strafe_speed": 8.0,
  "acceleration": 6.0
}
```

### Enable realistic gravity:
```json
"physics": {
  "enable_gravity": true,
  "gravity_strength": -9.8
}
```

### Change player color to red:
```json
"visual": {
  "color": { "r": 1.0, "g": 0.0, "b": 0.0 }
}
```

### Make player bigger:
```json
"visual": {
  "scale": 2.0
}
```

## Controls
- **W**: Move Forward (Z+ direction)
- **S**: Move Backward (Z- direction)  
- **A**: Move Left (X- direction)
- **D**: Move Right (X+ direction)
- **Space**: Move Up (Y+ direction)
- **C**: Move Down (Y- direction)

## Notes
- Changes take effect on next game restart
- Invalid values will fall back to safe defaults
- Check console output for configuration loading messages
- Coordinate system: X=Left/Right, Y=Up/Down, Z=Forward/Back