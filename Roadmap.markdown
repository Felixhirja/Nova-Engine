Nova-Engine Roadmap
====================

_Last updated: November 3, 2025_

This roadmap breaks down the next waves of engine work into digestible milestones.  Each milestone is scoped so it can be tackled in 1–4 week sprints while keeping an eye on the long-term vision captured in `TODO_LIST.txt`.

Recent highlights
-----------------

- ✅ **Configuration Management System**: Complete JSON-based config system with caching, validation, templates, and hot-reload support
- ✅ **Advanced ECS Query System**: Modern query builder with parallel execution, caching, and composition patterns (10-50x faster than legacy)
- ✅ **Spaceship Content System**: Modular ship assembly with JSON manifests, component-based architecture, and spawn bundles
- ✅ **Solar System Generation**: Procedural star system generation with realistic orbital mechanics and faction integration
- ✅ **Modern Shader Stack**: `ShaderProgram` with Blinn-Phong lighting, procedural skybox, and post-processing pipeline
- ✅ **Physics ECS Extensions**: Impulse-based collision response with deterministic simulation support
- ✅ **Actor Factory System**: Automatic actor registration and streamlined entity creation pipeline
- ✅ **Documentation Cleanup**: Removed 52+ redundant markdown files, consolidated essential technical documentation

Milestone 0 – Core Stability (COMPLETE ✅)
-----------------------------------------

- [x] GLFW + OpenGL rendering path up and running
- [x] Fixed-timestep main loop with input abstraction
- [x] Camera follow, presets, and zoom UX polishing
- [x] Document user controls (README update)
- [x] ECS V2 with archetype-based storage (10-50x performance improvement)
- [x] Advanced query system with parallel execution and caching
- [x] Configuration management system with validation and hot-reload
- [x] Actor factory with automatic registration
- [x] Continuous test suite with comprehensive coverage

Milestone 1 – Rendering & Visuals (IN PROGRESS 🔄)
---------------------------------

- [x] Replace immediate-mode drawing with retained-mode mesh abstraction
- [x] Basic lighting pass (ambient + single directional light) — powered by `shaders/core/basic.vert/frag`
- [x] Upload sprite/texture metadata to GPU once and reuse per frame
- [x] Prototype post-process pipeline (letterbox HUD overlay, bloom effects)
- [x] Author shared material system with instancing support for repeated geometry
- [x] Implement skybox renderer for space scene backgrounds
- [ ] Add GPU validation & debug markers to aid graphics driver debugging sessions
- [ ] Batch material parameter uploads to minimize state changes across draw calls
- [ ] Add renderdoc capture scripts + golden-frame comparisons to catch regressions
- [ ] Implement environment probe system for space scene reflections and dynamic lighting
- [ ] Migrate remaining immediate-mode rendering to modern VAO/VBO pipeline
- [ ] PBR (Physically-Based Rendering) material system for realistic ship surfaces

Milestone 2 – Simulation & Gameplay Systems (IN PROGRESS 🔄)
-------------------------------------------

- [x] Extend ECS with physics-friendly components (colliders, forces)
- [x] Implement collision response prototype (impulse-based solver shipped with `PhysicsSystem`)
- [x] Build deterministic random seed manager for reproducible scenarios (solar-system seeding utilities exist)
- [x] Modular spaceship assembly system with JSON-driven component configuration
- [x] Faction system with reputation tracking and dynamic relationships
- [ ] Author movement bounds/volumes from data files instead of hard-coded values
- [ ] Record deterministic replays for regression testing
- [ ] Layer gameplay events over ECS (damage, status effects, scripted triggers)
- [ ] Combat hooks for deterministic seed-based encounters
- [ ] Stand up AI behavior trees with reusable maneuver and targeting nodes
- [ ] Bake navigation grids/volumes per scene and expose steering helpers to ECS
- [ ] Create mission scripting layer with branching objectives and fail-states
- [ ] Player progression system (pilot ranks, skill specializations, reputation effects)

Milestone 3 – Tools & Pipeline (IN PROGRESS 🔄)
------------------------------

- [x] JSON-driven config system with schema validation and template support
- [x] Hot-reload system for configs, sprites, and camera presets
- [x] Configuration analytics and performance monitoring
- [x] Asset database with metadata tracking (`assets/asset_database.json`)
- [x] Add profiling hooks (frame time, ECS system timing via lifecycle analytics)
- [ ] Provide developer CLI commands for spawning test entities
- [ ] Create asset diff visualizer to compare binary resources across commits
- [ ] Integrate crash-dump symbolication into CI artifacts
- [ ] Write asset validation gate that rejects missing textures or mismatched GUIDs
- [ ] Spin up headless simulation runner for soak testing on CI
- [ ] Package editor utility scripts as VS Code tasks + custom keybindings
- [ ] In-game configuration editor with live preview and validation feedback

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

Milestone 5 – Planetary Landing & Exploration (NEW ✨)
----------------------------------------------------

- [x] Atmospheric entry physics with drag and heat calculation
- [x] Heat shield system with ablative protection
- [x] Landing gear deployment and ground detection
- [x] Landing zone management (spaceports, emergency sites, procedural)
- [x] EVA suit gameplay with oxygen management and life support
- [x] Jetpack systems for low-gravity mobility
- [x] Surface vehicle system (rovers, bikes, jetpacks, walkers)
- [x] Dynamic weather systems (storms, fog, extreme conditions)
- [x] Day/night cycles with planetary rotation
- [x] Resource scanning and detection systems
- [x] Mining equipment and resource extraction
- [x] Cave system framework for underground exploration
- [x] Flora and fauna encounter system
- [x] Surface base building (outposts, mining stations, habitats)
- [x] Environmental hazards (radiation, extreme temperatures, toxic atmosphere)
- [ ] Procedural terrain generation with heightmaps
- [ ] Biome systems for diverse planetary regions
- [ ] Ground-based missions (rescue, research, combat)
- [ ] Seismic activity and volcanic events
- [ ] Advanced cave network generation
- [ ] Modular base construction and expansion
- [ ] NPC surface populations and settlements

Long-Haul Outlook (pull from master backlog when ready)
-------------------------------------------------------

- [ ] Multiplayer experiment (lockstep or rollback prototype)
- [x] Procedural solar system demo scene (generation system complete, integration in progress)
- [ ] Asset conditioning tools (texture packing, shader compilation cache)
- [ ] Automated benchmarking harness that records frame-time histograms per commit
- [ ] Cross-platform launcher with patcher + telemetry opt-in dialog
- [ ] Steam Workshop-style mod packaging + sandbox verification tools
- [ ] Console certification checklist and platform abstraction investigation
- [ ] Cloud save + cross-progression roadmap (Steam, GOG, standalone)
- [x] Resource extraction and crafting system for ship component manufacturing (mining foundation complete)
- [ ] Dynamic economy with commodity trading and player-driven markets
- [ ] Territory control and player-owned stations (surface base framework complete)
- [ ] Advanced AI fleet tactics and squadron command systems

Tracking & Cadence
-------------------

- Keep `tests/` green by running `make test` before every commit.
- Update this roadmap at the end of each sprint; roll completed items into release notes.
- Use modern ECS V2 for all new systems; legacy compatibility maintained via facade.
- Leverage configuration system for all new game content and actor definitions.
- Monitor build times with `BUILD_SPEED_GUIDE.md` best practices (unity builds, incremental compilation).
- Document new systems in `docs/` using established patterns (architecture, quick reference, implementation guides).

