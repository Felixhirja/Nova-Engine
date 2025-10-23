# ECS Compatibility Facade

The legacy `EntityManager` now exposes an opt-in compatibility layer that bridges
existing APIs onto the archetype-backed `EntityManagerV2` implementation. This
facade can be activated at runtime by calling `EntityManager::EnableArchetypeFacade()`.

## When to enable the facade

* **Simulation bootstrap** – The `Simulation` class automatically enables the
  facade when the new scheduler flag is active. External callers can enable it
  before `Init()` if they intend to run with `SystemSchedulerV2` or otherwise
  depend on archetype-backed iteration.
* **Migration utilities** – Tests that validate migration behaviour now call
  `EnableArchetypeFacade()` before asserting on archetype storage.

Once enabled, entity creation/destruction, iteration helpers, and component
queries operate directly on archetype storage while still presenting the
legacy pointer-based API. Component slots are mirrored into the legacy
`components` map using non-owning aliases so existing code paths continue to
receive stable raw pointers.

## Unsupported pathways

* Components that are not registered with `ArchetypeManager` remain on the
  legacy storage path. They can be detected through
  `EntityManager::GetUnsupportedComponentTypes()` after calling
  `EnableArchetypeFacade()`. Custom modules should register their component
  types with `ArchetypeManager` (or extend the registration switch) before
  enabling the facade to benefit from archetype storage.
* The facade does not retrofit scheduler dependencies. Systems still deriving
  from `System` must be wrapped in `ecs::LegacySystemAdapter` to participate
  in `SystemSchedulerV2` execution.

## Scheduling parity

The simulation bootstrap can toggle the new scheduler via
`Simulation::SetUseSchedulerV2(bool)`. When enabled, systems are registered
through `ecs::LegacySystemAdapter`, preserving the manual update order while
executing through the archetype scheduler. Migration regression tests ensure
behaviour parity with the legacy `SystemManager` pipeline.

