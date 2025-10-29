# Risk Radar & Mitigation Plan

Nova Engine is entering a stretch of major platform work. The following radar highlights the highest-impact risks we are monitoring and the concrete mitigations that are either in-flight or queued next.

## Legacy OpenGL Dependency
- **Risk**: Core renderer still relies on legacy OpenGL 2.1 pathways that modern drivers increasingly throttle or deprecate.
- **Mitigations**:
  - Promote the 4.6 core profile path to the default renderer and run full regression coverage against all flagship scenes.
  - Maintain a compatibility backend that can fall back to 2.1 for aging GPUs, but gate it behind runtime capability detection to limit surface area.
  - Update the graphics capability probe so QA dashboards show which backend is selected on each test platform.

## Windows-Only Tooling
- **Risk**: Several authoring and diagnostic utilities depend on Windows-only shells or batch scripts, slowing down cross-platform contributors.
- **Mitigations**:
  - Provide Bash-based equivalents for each critical script (asset baking, ECS scaffolding, diagnostic captures).
  - Add a CI matrix entry that exercises both MSYS2 and native GNU toolchains to keep documentation honest.
  - Expand contributor docs to spell out when MSYS2 is required versus when native POSIX environments suffice.

## Input Backend Fragility
- **Risk**: Current input stack tightly couples GLFW, SDL, and platform APIs, which leads to undefined behavior when optional devices (e.g., gamepads) are absent or misconfigured.
- **Mitigations**:
  - Introduce a platform-agnostic abstraction layer that normalizes keyboard, mouse, and controller events before they reach gameplay systems.
  - Feature-detect device support at runtime and cache the results to avoid repeated probing.
  - Ensure the gameplay loop has a graceful "no gamepad" path with clear UI messaging instead of hard failures.

## ECS Transition Regression Risk
- **Risk**: Migrating the entity-component system introduces high churn in data ownership and scheduling, which can trigger structural hazards mid-frame.
- **Mitigations**:
  - Buffer structural commands so inserts/removals execute at safe synchronization points.
  - Run fuzz tests that randomize component mutation patterns and capture frame snapshots for comparison against golden baselines.
  - Automate regression suites that mix legacy and new ECS pathways during the transition period to surface desyncs early.
