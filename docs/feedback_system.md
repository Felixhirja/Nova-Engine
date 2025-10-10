# Visual and Audio Feedback System

## Overview
The feedback system provides immersive audio and visual responses to gameplay events such as damage, shield hits, weapon fire, and system alerts. It uses a decoupled event-driven architecture for flexibility and performance.

## Architecture

### Event System (`FeedbackEvent.h`)
- **FeedbackEventManager**: Singleton event bus for publishing feedback events
- **IFeedbackListener**: Interface for systems that respond to events
- **FeedbackEvent**: Event data structure containing type, severity, position, and magnitude
- **FeedbackEventType**: Enum of all event types (shield, hull, weapon, power, alerts)
- **AlertSeverity**: Info, Warning, Critical, Emergency

### Visual Feedback (`VisualFeedbackSystem.h/cpp`)
Handles particle effects and screen effects:

#### Particle Effects
- **Sparks**: Orange/yellow for hull damage, red for critical failures
- **Shield Impacts**: Blue/cyan radial burst on shield hits
- **Explosions**: Orange/red spherical burst for critical damage
- **Muzzle Flash**: Bright flash for weapon fire

#### Screen Effects
- **Screen Shake**: Camera shake intensity based on damage magnitude
- **Damage Decals**: (TODO) Scorch marks and dents on hull
- **HUD Overlays**: (TODO) Warning flashes and alert borders

#### Particle System Features
- Physics simulation (velocity, gravity, lifetime)
- Color gradients with alpha fade
- Size variation for visual interest
- Efficient pooling (pre-allocated up to 1000 particles)

### Audio Feedback (`AudioFeedbackSystem.h/cpp`)
Handles spatial and 2D audio with volume mixing:

#### Sound Categories
- **Shield Sounds**: Hit, depleted, recharging
- **Hull/Damage Sounds**: Impact, sparks, explosions, subsystem failures
- **Weapon Sounds**: Fire, overheat, ammo empty
- **Alarm Sounds**: Warning, critical, evacuate (looping)
- **UI Sounds**: Power diverted, beeps

#### Audio Features
- **3D Spatial Audio**: Distance-based attenuation for positional sounds
- **Volume Mixing**: Master, SFX, and Alarm volume controls
- **Looping Alarms**: Persistent warning/critical alarms
- **Sound Registry**: Pre-loaded audio clips with volume settings

#### Audio Backend
Current implementation is a stub with logging. Integration points for:
- OpenAL (3D spatial audio)
- SDL_mixer (cross-platform audio)
- FMOD (advanced audio engine)

## Event Flow

```
Game Event → System → FeedbackEventManager → Listeners → Audio/Visual Output
```

Example: Shield takes damage
1. `ShieldSystem::ApplyDamage()` is called
2. Creates `FeedbackEvent(ShieldHit, entityId)` with magnitude and position
3. Emits event to `FeedbackEventManager`
4. `VisualFeedbackSystem` spawns blue impact particles and triggers screen shake
5. `AudioFeedbackSystem` plays "shield_hit.wav" at 3D position

## Integration with Game Systems

### ShieldSystem
- **ShieldHit**: Emitted on damage absorption
- **ShieldDepleted**: Emitted when shields reach zero
- **ShieldRecharging**: Emitted during active recharge
- **ShieldFullyCharged**: Emitted when shields reach 100%
- **WarningLowShields**: Emitted when shields below 25%

### WeaponSystem
- **WeaponFired**: Emitted on successful weapon fire
- **AmmoEmpty**: Emitted when attempting to fire with no ammo

### EnergyManagementSystem
- **EnergyDiverted**: Emitted when power is diverted to a subsystem
- **PowerOverload**: (TODO) Emitted when power exceeds 110% capacity
- **PowerCritical**: (TODO) Emitted when power below 10%

### Future DamageSystem
- **HullDamage**: Emitted on direct hull impact
- **CriticalDamage**: Emitted on severe hull damage (>50 points)
- **SubsystemFailure**: Emitted when a component fails
- **HullBreach**: Emitted on catastrophic damage

## Usage Examples

### Emitting Events from Game Code
```cpp
#include "FeedbackEvent.h"

// Create event
FeedbackEvent event(FeedbackEventType::ShieldHit, entityId, AlertSeverity::Info);
event.magnitude = 50.0;  // Damage amount
event.x = hitPosition.x;
event.y = hitPosition.y;
event.z = hitPosition.z;
event.componentId = "shield_array_light";

// Emit to all listeners
FeedbackEventManager::Get().Emit(event);
```

### Registering Listeners
```cpp
// In Simulation::Init() or similar
auto visualFeedback = std::make_shared<VisualFeedbackSystem>();
auto audioFeedback = std::make_shared<AudioFeedbackSystem>();

audioFeedback->Initialize();

FeedbackEventManager::Get().Subscribe(visualFeedback);
FeedbackEventManager::Get().Subscribe(audioFeedback);
```

### Updating Systems
```cpp
// In main game loop
visualFeedback->Update(deltaTime);
visualFeedback->Render();

// Update listener position for 3D audio
audioFeedback->SetListenerPosition(camera.x, camera.y, camera.z);
audioFeedback->SetListenerOrientation(
    camera.forward.x, camera.forward.y, camera.forward.z,
    camera.up.x, camera.up.y, camera.up.z
);
```

## Asset Requirements

### Audio Files (to be created)
All audio files should be placed in `assets/audio/`:

**Shield Sounds:**
- `shield_hit.wav` - Metallic "bzzt" impact (0.3s)
- `shield_depleted.wav` - Electronic failure (0.5s)
- `shield_recharge.wav` - Powering up hum (1.0s loopable)

**Hull/Damage Sounds:**
- `hull_impact.wav` - Metallic thud (0.2s)
- `sparks.wav` - Electrical crackling (0.5s)
- `explosion.wav` - Deep boom (1.0s)
- `subsystem_failure.wav` - Short circuit (0.4s)

**Weapon Sounds:**
- `weapon_fire.wav` - Laser/plasma discharge (0.2s)
- `weapon_overheat.wav` - Sizzling overload (0.6s)
- `ammo_empty.wav` - Click/beep (0.1s)

**Alarm Sounds (looping):**
- `alarm_warning.wav` - Intermittent beep (2.0s loop)
- `alarm_critical.wav` - Rapid beeping (1.0s loop)
- `alarm_evacuate.wav` - Klaxon siren (3.0s loop)

**UI Sounds:**
- `power_diverted.wav` - Positive confirmation (0.2s)
- `beep_low.wav` - Low tone (0.1s)
- `beep_high.wav` - High tone (0.1s)

### Audio Production Guidelines
- **Format**: WAV 44.1kHz 16-bit stereo
- **Normalization**: Peak at -3dB to prevent clipping
- **Compression**: None (real-time mixing handles dynamics)
- **Style**: Sci-fi electronic, consistent sonic palette
- **Spatial**: Mono center for 3D positioned sounds

## Performance Considerations

### Particle System
- Maximum 1000 active particles (pre-allocated)
- Particles removed when lifetime expires
- Simple physics (velocity + gravity)
- Rendering optimized with batch rendering (TODO)

### Audio System
- Sound pooling to limit concurrent sounds
- Distance culling for far-away 3D sounds
- Priority system for critical alerts
- Efficient 3D spatialization

### Event System
- Minimal overhead (direct function calls)
- No heap allocations during event emission
- Listeners weakly coupled via interface

## TODO Items

### Short-term
- [ ] Integrate particle rendering with Viewport3D
- [ ] Add OpenAL backend for audio playback
- [ ] Implement damage decals on hull
- [ ] Add HUD alert overlay system

### Long-term
- [ ] Dynamic music system responding to combat intensity
- [ ] Advanced particle effects (trails, glows, distortions)
- [ ] Customizable audio profiles per ship class
- [ ] Haptic feedback for gamepad/VR controllers

## Testing

Test the feedback system with:
```cpp
// Test visual feedback
FeedbackEvent explosion(FeedbackEventType::CriticalDamage, 1);
explosion.magnitude = 100.0;
explosion.x = 0.0;
explosion.y = 0.0;
explosion.z = 5.0;
FeedbackEventManager::Get().Emit(explosion);

// Test audio feedback
FeedbackEvent alarm(FeedbackEventType::AlarmCritical, 1, AlertSeverity::Critical);
FeedbackEventManager::Get().Emit(alarm);
```

Monitor console output for audio playback confirmation and inspect particle count.
