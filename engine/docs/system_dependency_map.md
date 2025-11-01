# System Dependency Map

## Phase: Input

| System | Legacy Type | Component Access | System Dependencies |
| --- | --- | --- | --- |
| PlayerControlSystem | `13UnifiedSystem` | 16PlayerController (Read/Write)<br/>8Velocity (Read/Write) | None |

## Phase: Simulation

| System | Legacy Type | Component Access | System Dependencies |
| --- | --- | --- | --- |
| MovementSystem | `13UnifiedSystem` | 8Position (Read/Write)<br/>8Velocity (Read/Write)<br/>12Acceleration (Read) | None |
| LocomotionSystem | `13UnifiedSystem` | 19LocomotionComponent (Read/Write) | None |

