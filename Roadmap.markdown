Nova-Engine Roadmap
====================

_Last updated: October 10, 2025_

This roadmap breaks down the next waves of engine work into digestible milestones.  Each milestone is scoped so it can be tackled in 1–4 week sprints while keeping an eye on the long-term vision captured in `TODO_LIST.txt`.

Recent highlights
-----------------

- ✅ Modern shader stack landed (`ShaderProgram`, Blinn–Phong lighting, procedural skybox) with integration notes captured in `docs/shader_system_implementation_summary.md` and `SKYBOX_INTEGRATION_STATUS.md`.
- ✅ Particle renderer migrated to VAO/VBO, with the first UIBatcher rollout removing menu background immediate-mode calls (`docs/phase4_progress.md`, `docs/uibatcher_implementation.md`).
- ✅ Physics ECS extensions shipped, delivering impulse-based collision response and component coverage for gameplay systems (`docs/physics_implementation_summary.md`).
- ✅ Shared material system with instanced rendering reference dropped into source package—see `docs/material_system_summary.md` for contents and integration hooks.

Milestone 0 – Core Stability (in progress)
-----------------------------------------

- [x] GLFW + OpenGL rendering path up and running
- [x] Fixed-timestep main loop with input abstraction
- [x] Camera follow, presets, and zoom UX polishing
- [ ] Document user controls (README update)
- [x] Continuous test suite hooked into CI (GitHub Actions or similar)

Milestone 1 – Rendering & Visuals
---------------------------------

- [x] Replace immediate-mode drawing with retained-mode mesh abstraction
  - ParticleRenderer fully live; UIBatcher handling menu background with HUD batching queued next.
- [x] Basic lighting pass (ambient + single directional light) — powered by `shaders/core/basic.vert/frag`.
- [ ] Upload sprite/texture metadata to GPU once and reuse per frame
- [x] Prototype post-process pipeline (letterbox HUD overlay, simple bloom toggle)
- [x] Author shared material system with instancing support for repeated geometry (see `docs/material_system_summary.md`)
- [ ] Add GPU validation & debug markers to aid graphics driver debugging sessions
- [ ] Batch material parameter uploads to minimize state changes across draw calls
- [ ] Add renderdoc capture scripts + golden-frame comparisons to catch regressions
- [ ] Implement skybox and environment probe system for space scene reflections (skybox renderer implemented; runtime validation blocked on GLFW crash; probes TBD)

Milestone 2 – Simulation & Gameplay Systems
-------------------------------------------

- [x] Extend ECS with physics-friendly components (colliders, forces)
- [ ] Author movement bounds/volumes from data files instead of hard-coded values
- [x] Implement collision response prototype (swept AABB) — impulse-based solver shipped with `PhysicsSystem`.
- [ ] Record deterministic replays for regression testing
- [ ] Layer gameplay events over ECS (damage, status effects, scripted triggers)
- [ ] Build deterministic random seed manager for reproducible combat scenarios (solar-system seeding utilities exist; combat hooks still needed)
- [ ] Stand up AI behavior trees with reusable maneuver and targeting nodes
- [ ] Bake navigation grids/volumes per scene and expose steering helpers to ECS
- [ ] Create mission scripting layer with branching objectives and fail-states

Milestone 3 – Tools & Pipeline
------------------------------

- [ ] YAML/JSON-driven scene description loader
- [ ] Hot-reload sprite sheets and camera presets from disk
- [ ] Add profiling hooks (frame time, ECS system timing)
- [ ] Provide developer CLI commands for spawning test entities
- [ ] Create asset diff visualizer to compare binary resources across commits
- [ ] Integrate crash-dump symbolication into CI artifacts
- [ ] Write asset validation gate that rejects missing textures or mismatched GUIDs
- [ ] Spin up headless simulation runner for soak testing on CI
- [ ] Package editor utility scripts as VS Code tasks + custom keybindings

Milestone 4 – User Experience & Polish
--------------------------------------

- [ ] HUD polish: integrate FPS, zoom, target-lock indicators stylistically
- [ ] Audio layer: hook up SFX + background loop (OpenAL or SDL_mixer)
- [ ] Settings menu with key rebinding and sensitivity sliders
- [ ] Screenshots/video capture helpers for marketing/documentation
- [ ] Tutorial fly-through that introduces controls and ship systems step-by-step
- [ ] Ship customization presets with saved loadouts in user profile directory
- [ ] Achievement-style pilot log that tracks mission history and unlocks cosmetics
- [ ] Localization scaffolding with font fallback packs and string tables
- [ ] Accessibility pass: configurable contrast modes, screen-shake intensity slider

Long-Haul Outlook (pull from master backlog when ready)
-------------------------------------------------------

- [ ] Multiplayer experiment (lockstep or rollback prototype)
- [ ] Procedural solar system demo scene
- [ ] Asset conditioning tools (texture packing, shader compilation cache)
- [ ] Automated benchmarking harness that records frame-time histograms per commit
- [ ] Cross-platform launcher with patcher + telemetry opt-in dialog
- [ ] Steam Workshop-style mod packaging + sandbox verification tools
- [ ] Console certification checklist and platform abstraction investigation
- [ ] Cloud save + cross-progression roadmap (Steam, GOG, standalone)

Tracking & Cadence
-------------------

- Keep `tests/` green by running `make test` before every commit.
- Update this roadmap at the end of each sprint; roll completed items into release notes.
- Sync with `todo_camera.txt` and `TODO_LIST.txt` monthly so tactical camera/UI work stays aligned with the strategic plan.

