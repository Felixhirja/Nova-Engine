// Unity build file for Nova Engine
// Combines multiple source files for faster compilation
// Usage: make UNITY_BUILD=1

#ifdef UNITY_BUILD

// Core engine files
#include "engine/MainLoop.cpp"
#include "engine/Viewport3D.cpp"
#include "engine/Simulation.cpp"
#include "engine/Input.cpp"
#include "engine/Transform.cpp"
#include "engine/Mesh.cpp"
#include "engine/SimpleJson.cpp"

// ECS system
#include "engine/ecs/EntityManager.cpp"
#include "engine/ecs/ArchetypeManager.cpp"
#include "engine/ecs/TransitionPlan.cpp"
#include "engine/ecs/System.cpp"

// Graphics system
#include "engine/graphics/EntityRenderer.cpp"
#include "engine/graphics/PrimitiveMesh.cpp"
#include "engine/graphics/MaterialLibrary.cpp"

// Basic entities
#include "entities/Spaceship.cpp"
#include "entities/SpaceStation.cpp"
#include "entities/Projectile.cpp"

#endif // UNITY_BUILD