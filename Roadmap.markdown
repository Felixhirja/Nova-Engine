Star-Engine Roadmap
====================

This roadmap breaks down the next waves of engine work into digestible milestones.  Each milestone is scoped so it can be tackled in 1–4 week sprints while keeping an eye on the long-term vision captured in `TODO_LIST.txt`.

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
- [ ] Basic lighting pass (ambient + single directional light)
- [ ] Upload sprite/texture metadata to GPU once and reuse per frame
- [x] Prototype post-process pipeline (letterbox HUD overlay, simple bloom toggle)

Milestone 2 – Simulation & Gameplay Systems
-------------------------------------------

- [x] Extend ECS with physics-friendly components (colliders, forces)
- [ ] Author movement bounds/volumes from data files instead of hard-coded values
- [ ] Implement collision response prototype (swept AABB)
- [ ] Record deterministic replays for regression testing

Milestone 3 – Tools & Pipeline
------------------------------

- [ ] YAML/JSON-driven scene description loader
- [ ] Hot-reload sprite sheets and camera presets from disk
- [ ] Add profiling hooks (frame time, ECS system timing)
- [ ] Provide developer CLI commands for spawning test entities

Milestone 4 – User Experience & Polish
--------------------------------------

- [ ] HUD polish: integrate FPS, zoom, target-lock indicators stylistically
- [ ] Audio layer: hook up SFX + background loop (OpenAL or SDL_mixer)
- [ ] Settings menu with key rebinding and sensitivity sliders
- [ ] Screenshots/video capture helpers for marketing/documentation

Long-Haul Outlook (pull from master backlog when ready)
-------------------------------------------------------

- [ ] Multiplayer experiment (lockstep or rollback prototype)
- [ ] Procedural solar system demo scene
- [ ] Asset conditioning tools (texture packing, shader compilation cache)

Tracking & Cadence
-------------------

- Keep `tests/` green by running `make test` before every commit.
- Update this roadmap at the end of each sprint; roll completed items into release notes.
- Sync with `todo_camera.txt` and `TODO_LIST.txt` monthly so tactical camera/UI work stays aligned with the strategic plan.

