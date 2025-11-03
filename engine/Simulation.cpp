// TODO: Simulation System Enhancement Roadmap
// 
// ═══════════════════════════════════════════════════════════════════════════════════
// SIMULATION SYSTEM - COMPREHENSIVE IMPROVEMENT PLAN
// ═══════════════════════════════════════════════════════════════════════════════════
//
// 🌍 PHYSICS & SIMULATION
// [ ] Advanced Physics: Multi-threaded physics simulation with spatial partitioning
// [ ] Collision Detection: Optimized broad-phase and narrow-phase collision detection
// [ ] Fluid Dynamics: Realistic fluid simulation for space and atmospheric environments
// [ ] Gravitational Fields: N-body gravitational simulation for realistic orbital mechanics
// [ ] Atmospheric Simulation: Realistic atmospheric effects and aerodynamics
// [ ] Temperature Simulation: Heat transfer and thermal management systems
// [ ] Electromagnetic Fields: EM field simulation for shield and weapon interactions
// [ ] Particle Systems: Advanced particle simulation for debris and effects
// [ ] Soft Body Physics: Deformable objects and materials simulation
// [ ] Constraint Systems: Advanced constraint-based physics for complex mechanisms
//
// 🚀 ENTITY & MOVEMENT SYSTEMS
// [ ] Entity Pooling: Object pooling for frequently created/destroyed entities
// [ ] Movement Prediction: Client-side prediction for smooth networked movement
// [ ] Locomotion AI: Intelligent locomotion system with pathfinding integration
// [ ] Movement Profiles: Configurable movement profiles for different entity types
// [ ] Surface Adaptation: Dynamic adaptation to different surface types and conditions
// [ ] Movement Validation: Server-side validation of all movement commands
// [ ] Movement Recording: Record and replay movement for debugging and analysis
// [ ] Kinematic Chains: Support for complex kinematic structures (mechs, robots)
// [ ] Movement Smoothing: Advanced interpolation and extrapolation for smooth movement
// [ ] Movement Constraints: Physical and gameplay constraints on entity movement
//
// ⚡ PERFORMANCE OPTIMIZATIONS
// [ ] Spatial Indexing: Efficient spatial data structures for fast proximity queries
// [ ] Level of Detail: LOD system for physics simulation based on distance/importance
// [ ] Update Scheduling: Intelligent scheduling of entity updates based on importance
// [ ] Simulation Culling: Cull simulation updates for entities outside view/interaction
// [ ] Batch Processing: Batch similar operations for better cache performance
// [ ] Memory Management: Optimize memory layout and reduce allocations
// [ ] SIMD Operations: Use SIMD instructions for vector and matrix operations
// [ ] GPU Acceleration: Offload appropriate calculations to GPU compute shaders
// [ ] Temporal Coherence: Leverage frame-to-frame coherence for optimization
// [ ] Adaptive Timesteps: Dynamic timestep adjustment based on simulation complexity
//
// 🎮 GAMEPLAY SYSTEMS
// [ ] Event System: Comprehensive event system for gameplay interactions
// [ ] State Management: Advanced state management for complex game states
// [ ] Save/Load System: Complete save/load functionality for simulation state
// [ ] Replay System: Record and replay entire simulation sessions
// [ ] Scripting Integration: Lua/Python scripting support for gameplay logic
// [ ] Mission System: Dynamic mission generation and management
// [ ] Faction System: Complex faction relationships and interactions
// [ ] Economy System: Dynamic economy simulation with supply and demand
// [ ] Procedural Generation: Procedural content generation for entities and environments
// [ ] Balancing System: Automatic gameplay balancing based on player performance
//
// 🔗 NETWORKING & MULTIPLAYER
// [ ] Network Synchronization: Deterministic simulation for multiplayer support
// [ ] State Compression: Efficient compression of simulation state for networking
// [ ] Client Prediction: Client-side prediction with server reconciliation
// [ ] Interest Management: Network interest management for large-scale simulations
// [ ] Delta Compression: Delta compression for efficient state updates
// [ ] Network Security: Anti-cheat measures and input validation
// [ ] Lag Compensation: Lag compensation techniques for smooth multiplayer
// [ ] Network Diagnostics: Comprehensive network performance monitoring
// [ ] Rollback Networking: Rollback netcode for competitive multiplayer
// [ ] P2P Support: Peer-to-peer networking for distributed simulations
//
// 🔧 DEBUGGING & TOOLS
// [ ] Simulation Profiler: Detailed profiling of simulation performance
// [ ] Debug Visualization: Visual debugging tools for physics and AI
// [ ] Simulation Inspector: Runtime inspection of simulation state
// [ ] Performance Metrics: Real-time performance metrics and analysis
// [ ] Memory Profiling: Track memory usage and detect leaks
// [ ] Error Recovery: Graceful handling of simulation errors and edge cases
// [ ] Unit Testing: Comprehensive unit tests for all simulation components
// [ ] Integration Testing: Automated integration testing of simulation systems
// [ ] Stress Testing: Stress testing with large numbers of entities
// [ ] Regression Testing: Automated regression testing for simulation accuracy

#include "Simulation.h"
#include "ecs/LegacySystemAdapter.h"
#include "ecs/System.h"  // For UnifiedSystem
#include "EntityFactory.h"  // Auto-loading entity creation
#include "gameplay/GameplayEventSystem.h"
#include "gameplay/MissionScriptSystem.h"
#include "TargetingSystem.h"
#include "CameraSystem.h"
#include "Entities.h"  // Auto-generated includes for all entity classes

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

MovementBounds CreateDefaultMovementBounds() {
    MovementBounds bounds;
    bounds.minX = -5.0;
    bounds.maxX = 5.0;
    bounds.clampX = true;
    bounds.minY = -5.0;
    bounds.maxY = 5.0;
    bounds.clampY = true;
    bounds.minZ = 0.0;
    bounds.maxZ = 5.0;
    bounds.clampZ = true;
    return bounds;
}

constexpr unsigned int kCollisionLayerEnvironment = 1u << 0;
constexpr unsigned int kCollisionLayerPlayer = 1u << 1;
constexpr double kEnvironmentWallThickness = 0.5;
constexpr double kDefaultEnvironmentSpan = 50.0;
constexpr double kDefaultEnvironmentHeight = 10.0;

struct EnvironmentColliderDefinition {
    double centerX = 0.0;
    double centerY = 0.0;
    double centerZ = 0.0;
    double sizeX = 1.0;
    double sizeY = 1.0;
    double sizeZ = 1.0;
    LocomotionSurfaceType surfaceType = LocomotionSurfaceType::PlanetaryGround;
    bool overridesProfile = false;
    SurfaceMovementProfile movementProfile;
    bool isHazard = false;
    HazardModifier hazardModifier;
};

double ComputeSpan(double minValue, double maxValue, double fallback) {
    if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
        return fallback;
    }
    double span = maxValue - minValue;
    return (span > 0.0) ? span : fallback;
}

double ComputeCenter(double minValue, double maxValue) {
    if (!std::isfinite(minValue) || !std::isfinite(maxValue)) {
        return 0.0;
    }
    return (minValue + maxValue) * 0.5;
}

std::vector<EnvironmentColliderDefinition> BuildEnvironmentFromBounds(const MovementBounds& bounds) {
    std::vector<EnvironmentColliderDefinition> colliders;

    double spanX = ComputeSpan(bounds.minX, bounds.maxX, kDefaultEnvironmentSpan);
    double spanY = ComputeSpan(bounds.minY, bounds.maxY, kDefaultEnvironmentSpan);
    double spanZ = ComputeSpan(bounds.minZ, bounds.maxZ, kDefaultEnvironmentHeight);

    double centerX = ComputeCenter(bounds.minX, bounds.maxX);
    double centerY = ComputeCenter(bounds.minY, bounds.maxY);
    double centerZ = ComputeCenter(bounds.minZ, bounds.maxZ);

    if (bounds.clampZ && std::isfinite(bounds.minZ)) {
        EnvironmentColliderDefinition floor;
        floor.centerX = centerX;
        floor.centerY = centerY;
        floor.centerZ = bounds.minZ - kEnvironmentWallThickness * 0.5;
        floor.sizeX = spanX + 2.0 * kEnvironmentWallThickness;
        floor.sizeY = spanY + 2.0 * kEnvironmentWallThickness;
        floor.sizeZ = kEnvironmentWallThickness;
        floor.surfaceType = LocomotionSurfaceType::PlanetaryGround;
        colliders.push_back(floor);
    }

    if (bounds.clampZ && std::isfinite(bounds.maxZ)) {
        EnvironmentColliderDefinition ceiling;
        ceiling.centerX = centerX;
        ceiling.centerY = centerY;
        ceiling.centerZ = bounds.maxZ + kEnvironmentWallThickness * 0.5;
        ceiling.sizeX = spanX + 2.0 * kEnvironmentWallThickness;
        ceiling.sizeY = spanY + 2.0 * kEnvironmentWallThickness;
        ceiling.sizeZ = kEnvironmentWallThickness;
        ceiling.surfaceType = LocomotionSurfaceType::Spacewalk;
        ceiling.overridesProfile = true;
        ceiling.movementProfile.gravityMultiplier = 0.05;
        ceiling.movementProfile.accelerationMultiplier = 0.6;
        ceiling.movementProfile.decelerationMultiplier = 0.6;
        ceiling.movementProfile.maxSpeedMultiplier = 0.85;
        ceiling.isHazard = true;
        ceiling.hazardModifier.gravityMultiplier = 0.5;
        ceiling.hazardModifier.speedMultiplier = 0.75;
        ceiling.hazardModifier.accelerationMultiplier = 0.6;
        ceiling.hazardModifier.heatGainRate = 10.0;
        colliders.push_back(ceiling);
    }

    double wallHeight = bounds.clampZ && std::isfinite(bounds.minZ) && std::isfinite(bounds.maxZ)
                            ? std::max(spanZ, kEnvironmentWallThickness)
                            : kDefaultEnvironmentHeight;
    double wallCenterZ;
    if (bounds.clampZ && std::isfinite(bounds.minZ) && std::isfinite(bounds.maxZ)) {
        wallCenterZ = (bounds.minZ + bounds.maxZ) * 0.5;
    } else if (std::isfinite(bounds.minZ)) {
        wallCenterZ = bounds.minZ + wallHeight * 0.5;
    } else if (std::isfinite(bounds.maxZ)) {
        wallCenterZ = bounds.maxZ - wallHeight * 0.5;
    } else {
        wallCenterZ = centerZ;
    }

    if (bounds.clampX && std::isfinite(bounds.maxX)) {
        EnvironmentColliderDefinition wall;
        wall.centerX = bounds.maxX + kEnvironmentWallThickness * 0.5;
        wall.centerY = centerY;
        wall.centerZ = wallCenterZ;
        wall.sizeX = kEnvironmentWallThickness;
        wall.sizeY = spanY + 2.0 * kEnvironmentWallThickness;
        wall.sizeZ = wallHeight;
        wall.surfaceType = LocomotionSurfaceType::ZeroGInterior;
        wall.overridesProfile = true;
        wall.movementProfile.gravityMultiplier = 0.15;
        wall.movementProfile.accelerationMultiplier = 0.75;
        wall.movementProfile.decelerationMultiplier = 0.75;
        wall.movementProfile.maxSpeedMultiplier = 0.9;
        colliders.push_back(wall);
    }

    if (bounds.clampX && std::isfinite(bounds.minX)) {
        EnvironmentColliderDefinition wall;
        wall.centerX = bounds.minX - kEnvironmentWallThickness * 0.5;
        wall.centerY = centerY;
        wall.centerZ = wallCenterZ;
        wall.sizeX = kEnvironmentWallThickness;
        wall.sizeY = spanY + 2.0 * kEnvironmentWallThickness;
        wall.sizeZ = wallHeight;
        wall.surfaceType = LocomotionSurfaceType::ZeroGInterior;
        wall.overridesProfile = true;
        wall.movementProfile.gravityMultiplier = 0.15;
        wall.movementProfile.accelerationMultiplier = 0.75;
        wall.movementProfile.decelerationMultiplier = 0.75;
        wall.movementProfile.maxSpeedMultiplier = 0.9;
        colliders.push_back(wall);
    }

    if (bounds.clampY && std::isfinite(bounds.maxY)) {
        EnvironmentColliderDefinition wall;
        wall.centerX = centerX;
        wall.centerY = bounds.maxY + kEnvironmentWallThickness * 0.5;
        wall.centerZ = wallCenterZ;
        wall.sizeX = spanX + 2.0 * kEnvironmentWallThickness;
        wall.sizeY = kEnvironmentWallThickness;
        wall.sizeZ = wallHeight;
        wall.surfaceType = LocomotionSurfaceType::ZeroGInterior;
        wall.overridesProfile = true;
        wall.movementProfile.gravityMultiplier = 0.15;
        wall.movementProfile.accelerationMultiplier = 0.75;
        wall.movementProfile.decelerationMultiplier = 0.75;
        wall.movementProfile.maxSpeedMultiplier = 0.9;
        colliders.push_back(wall);
    }

    if (bounds.clampY && std::isfinite(bounds.minY)) {
        EnvironmentColliderDefinition wall;
        wall.centerX = centerX;
        wall.centerY = bounds.minY - kEnvironmentWallThickness * 0.5;
        wall.centerZ = wallCenterZ;
        wall.sizeX = spanX + 2.0 * kEnvironmentWallThickness;
        wall.sizeY = kEnvironmentWallThickness;
        wall.sizeZ = wallHeight;
        wall.surfaceType = LocomotionSurfaceType::ZeroGInterior;
        wall.overridesProfile = true;
        wall.movementProfile.gravityMultiplier = 0.15;
        wall.movementProfile.accelerationMultiplier = 0.75;
        wall.movementProfile.decelerationMultiplier = 0.75;
        wall.movementProfile.maxSpeedMultiplier = 0.9;
        colliders.push_back(wall);
    }

    return colliders;
}

std::string TrimString(const std::string& value) {
    size_t start = 0;
    size_t end = value.size();
    while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

bool ParseBoolString(const std::string& rawValue, bool& outValue) {
    std::string value = TrimString(rawValue);
    if (value.empty()) {
        return false;
    }

    if (!value.empty() && (value[0] == '#' || value[0] == ';')) {
        return false;
    }

    auto commentPos = value.find_first_of("#;");
    if (commentPos != std::string::npos) {
        value = Trim(value.substr(0, commentPos));
    }

    std::string lowered;
    lowered.reserve(value.size());
    for (char c : value) {
        lowered.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (lowered == "true" || lowered == "1" || lowered == "yes" || lowered == "on") {
        outValue = true;
        return true;
    }
    if (lowered == "false" || lowered == "0" || lowered == "no" || lowered == "off") {
        outValue = false;
        return true;
    }
    return false;
}

bool ParseDoubleString(const std::string& rawValue, double& outValue) {
    std::string value = TrimString(rawValue);
    if (value.empty()) {
        return false;
    }

    if (!value.empty() && (value[0] == '#' || value[0] == ';')) {
        return false;
    }

    auto commentPos = value.find_first_of("#;");
    if (commentPos != std::string::npos) {
        value = Trim(value.substr(0, commentPos));
    }

    try {
        size_t idx = 0;
        outValue = std::stod(value, &idx);
        return idx == value.size();
    } catch (const std::exception&) {
        return false;
    }
}

bool ParseMovementBoundsStream(std::istream& input, std::unordered_map<std::string, MovementBounds>& outProfiles) {
    std::string line;
    std::string currentProfile;
    MovementBounds currentBounds;
    bool inProfile = false;

    auto commitProfile = [&]() {
        if (!currentProfile.empty()) {
            outProfiles[currentProfile] = currentBounds;
        }
    };

    while (std::getline(input, line)) {
    std::string trimmed = TrimString(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (inProfile) {
                commitProfile();
            }
            currentProfile = TrimString(trimmed.substr(1, trimmed.size() - 2));
            currentBounds = MovementBounds();
            inProfile = true;
            continue;
        }

        auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos || !inProfile) {
            continue;
        }

    std::string key = TrimString(trimmed.substr(0, equalsPos));
    std::string value = TrimString(trimmed.substr(equalsPos + 1));

        if (key.empty()) {
            continue;
        }

        double numericValue = 0.0;
        bool boolValue = false;
        if (key == "minX") {
            if (ParseDoubleString(value, numericValue)) {
                currentBounds.minX = numericValue;
            }
        } else if (key == "maxX") {
            if (ParseDoubleString(value, numericValue)) {
                currentBounds.maxX = numericValue;
            }
        } else if (key == "minY") {
            if (ParseDoubleString(value, numericValue)) {
                currentBounds.minY = numericValue;
            }
        } else if (key == "maxY") {
            if (ParseDoubleString(value, numericValue)) {
                currentBounds.maxY = numericValue;
            }
        } else if (key == "minZ") {
            if (ParseDoubleString(value, numericValue)) {
                currentBounds.minZ = numericValue;
            }
        } else if (key == "maxZ") {
            if (ParseDoubleString(value, numericValue)) {
                currentBounds.maxZ = numericValue;
            }
        } else if (key == "clampX") {
            if (ParseBoolString(value, boolValue)) {
                currentBounds.clampX = boolValue;
            }
        } else if (key == "clampY") {
            if (ParseBoolString(value, boolValue)) {
                currentBounds.clampY = boolValue;
            }
        } else if (key == "clampZ") {
            if (ParseBoolString(value, boolValue)) {
                currentBounds.clampZ = boolValue;
            }
        }
    }

    if (inProfile) {
        commitProfile();
    }

    return !outProfiles.empty();
}

bool ParseMovementParametersStream(std::istream& input, std::unordered_map<std::string, MovementParameters>& outProfiles) {
    std::string line;
    std::string currentProfile;
    MovementParameters currentParams;
    bool inProfile = false;

    auto commitProfile = [&]() {
        if (!currentProfile.empty()) {
            outProfiles[currentProfile] = currentParams;
        }
    };

    while (std::getline(input, line)) {
    std::string trimmed = TrimString(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (inProfile) {
                commitProfile();
            }
            currentProfile = TrimString(trimmed.substr(1, trimmed.size() - 2));
            currentParams = MovementParameters();
            inProfile = true;
            continue;
        }

        auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos || !inProfile) {
            continue;
        }

    std::string key = TrimString(trimmed.substr(0, equalsPos));
    std::string value = TrimString(trimmed.substr(equalsPos + 1));

        if (key.empty()) {
            continue;
        }

        double numericValue = 0.0;
        if (ParseDoubleString(value, numericValue)) {
            if (key == "strafeAcceleration") {
                currentParams.strafeAcceleration = numericValue;
            } else if (key == "forwardAcceleration") {
                currentParams.forwardAcceleration = numericValue;
            } else if (key == "backwardAcceleration") {
                currentParams.backwardAcceleration = numericValue;
            } else if (key == "strafeDeceleration") {
                currentParams.strafeDeceleration = numericValue;
            } else if (key == "forwardDeceleration") {
                currentParams.forwardDeceleration = numericValue;
            } else if (key == "backwardDeceleration") {
                currentParams.backwardDeceleration = numericValue;
            } else if (key == "strafeMaxSpeed") {
                currentParams.strafeMaxSpeed = numericValue;
            } else if (key == "forwardMaxSpeed") {
                currentParams.forwardMaxSpeed = numericValue;
            } else if (key == "backwardMaxSpeed") {
                currentParams.backwardMaxSpeed = numericValue;
            } else if (key == "friction") {
                currentParams.friction = numericValue;
            }
        }
    }

    if (inProfile) {
        commitProfile();
    }

    return !outProfiles.empty();
}

bool IsPathRelative(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return false;
    }
    if (path.size() > 1 && path[1] == ':') {
        return false;
    }
    return true;
}

std::unordered_map<std::string, MovementBounds> LoadMovementBoundsProfiles(const std::string& path) {
    std::unordered_map<std::string, MovementBounds> profiles;
    if (path.empty()) {
        return profiles;
    }

    std::vector<std::string> candidates;
    candidates.push_back(path);
    if (IsPathRelative(path)) {
        candidates.push_back("../" + path);
        candidates.push_back("../../" + path);
    }

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (!file.is_open()) {
            continue;
        }

        std::unordered_map<std::string, MovementBounds> parsedProfiles;
        if (ParseMovementBoundsStream(file, parsedProfiles)) {
            return parsedProfiles;
        }
    }

    return profiles;
}

std::unordered_map<std::string, MovementParameters> LoadMovementParametersProfiles(const std::string& path) {
    std::unordered_map<std::string, MovementParameters> profiles;
    if (path.empty()) {
        return profiles;
    }

    std::vector<std::string> candidates;
    candidates.push_back(path);
    if (IsPathRelative(path)) {
        candidates.push_back("../" + path);
        candidates.push_back("../../" + path);
    }

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (!file.is_open()) {
            continue;
        }

        std::unordered_map<std::string, MovementParameters> parsedProfiles;
        if (ParseMovementParametersStream(file, parsedProfiles)) {
            return parsedProfiles;
        }
    }

    return profiles;
}

MovementParameters ResolveMovementParameters(const MovementParameters& fallback, const std::string& path, const std::string& profile) {
    auto profiles = LoadMovementParametersProfiles(path);
    if (profiles.empty()) {
        return fallback;
    }

    if (!profile.empty()) {
        auto it = profiles.find(profile);
        if (it != profiles.end()) {
            return it->second;
        }
    }

    auto defaultIt = profiles.find("default");
    if (defaultIt != profiles.end()) {
        return defaultIt->second;
    }

    return profiles.begin()->second;
}

MovementBounds ResolveMovementBounds(const MovementBounds& fallback, const std::string& path, const std::string& profile) {
    auto profiles = LoadMovementBoundsProfiles(path);
    if (profiles.empty()) {
        return fallback;
    }

    if (!profile.empty()) {
        auto it = profiles.find(profile);
        if (it != profiles.end()) {
            return it->second;
        }
    }

    auto defaultIt = profiles.find("default");
    if (defaultIt != profiles.end()) {
        return defaultIt->second;
    }

    return profiles.begin()->second;
}

}  // namespace

Simulation::Simulation()
    : inputForward(false),
      inputBackward(false),
      inputUp(false),
      inputDown(false),
      inputStrafeLeft(false),
      inputStrafeRight(false),
      inputCameraYaw(0.0),
      inputSprint(false),
      inputCrouch(false),
      inputSlide(false),
      inputBoost(false),
      prevJumpHeld(false),
      useThrustMode(false),
      inputLeft(false),
      inputRight(false),
      schedulerV2_(),
      useSchedulerV2_(false),
      movementBoundsConfig(CreateDefaultMovementBounds()),
      movementParametersConfigPath("assets/config/player_movement.ini"),
      movementParametersProfile("default"),
      useMovementParametersFile(true),
      movementBoundsConfigPath("assets/config/movement_bounds.ini"),
      movementBoundsProfile("default"),
      useMovementBoundsFile(true),
      elapsedTimeSeconds_(0.0) {
    
    // TODO: Enhanced Simulation constructor
    // [ ] Configuration Loading: Load simulation settings from configuration files
    // [ ] Physics Engine Selection: Runtime selection of physics engine based on requirements
    // [ ] Memory Budgeting: Set up memory budgets for different simulation components
    // [ ] System Registration: Automatic registration of simulation systems
    // [ ] Resource Pre-allocation: Pre-allocate frequently used simulation resources
    // [ ] Profiler Integration: Set up profiling for simulation performance analysis
    // [ ] Event System Setup: Initialize comprehensive event system for simulation
    // [ ] Error Handler Setup: Set up error handling and recovery mechanisms
    // [ ] Threading Configuration: Configure threading model for simulation systems
    // [ ] Platform Optimization: Platform-specific optimizations during construction
    
    activeEm = &em;
    randomManager_.SetGlobalSeed(0u);
    replayRecorder_.StopRecording();
    replayPlayer_.StopPlayback();
}

Simulation::~Simulation() {}

void Simulation::SetUseSchedulerV2(bool enabled) {
    if (useSchedulerV2_ == enabled) {
        return;
    }

    useSchedulerV2_ = enabled;
    schedulerConfigured_ = false;

    if (!useSchedulerV2_) {
        schedulerV2_.Clear();
    }
}

void Simulation::Init(EntityManager* externalEm) {
    // TODO: Enhanced initialization system
    // [ ] Parallel Initialization: Initialize independent systems concurrently
    // [ ] Dependency Resolution: Proper dependency-based initialization order
    // [ ] Error Recovery: Graceful handling of initialization failures
    // [ ] Resource Validation: Validate all required resources are available
    // [ ] System Health Checks: Verify all systems are properly initialized
    // [ ] Performance Profiling: Profile initialization performance
    // [ ] Memory Management: Optimize memory allocation during initialization
    // [ ] Configuration Validation: Validate all configuration settings
    // [ ] Plugin Loading: Load and initialize simulation plugins
    // [ ] State Restoration: Restore previous simulation state if needed
    
    position = 0.0;
    std::cout << "Simulation initialized. position=" << position << std::endl;

    activeEm = externalEm ? externalEm : &em;
    EntityManager* useEm = activeEm;

    elapsedTimeSeconds_ = 0.0;
    replayPlayer_.StopPlayback();
    if (replayRecorder_.IsRecording()) {
        replayRecorder_.StartRecording(randomManager_.GetGlobalSeed());
    }
    randomManager_.RegisterNamedStream("combat", randomManager_.GetGlobalSeed() + 1u);

    schedulerConfigured_ = false;

    DestroyEnvironmentColliders(*useEm);

    if (!externalEm) {
        useEm->Clear();
    }

    systemManager.Clear();
    systemManager.SetDocumentationOutputPath("engine/docs/system_dependency_map.md");
    
    // Essential systems for basic movement - save pointers for direct invocation
    playerControlSystem = &systemManager.RegisterUnifiedSystem(SystemType::PlayerControl);
    movementSystem = &systemManager.RegisterUnifiedSystem(SystemType::Movement);
    locomotionSystem = &systemManager.RegisterUnifiedSystem(SystemType::Locomotion);
    
    // Advanced systems - only register if enabled for performance
    if (enableAdvancedSystems) {
        systemManager.RegisterUnifiedSystem(SystemType::ShipAssembly);
        systemManager.RegisterUnifiedSystem(SystemType::SpaceshipPhysics);
        systemManager.RegisterUnifiedSystem(SystemType::Animation);
        systemManager.RegisterUnifiedSystem(SystemType::Targeting);
        systemManager.RegisterUnifiedSystem(SystemType::Weapon);
        systemManager.RegisterUnifiedSystem(SystemType::Shield);
        auto& behaviorSystem = systemManager.RegisterUnifiedSystem(SystemType::BehaviorTree);
        behaviorSystem.SetRandomManager(&randomManager_);
        systemManager.RegisterUnifiedSystem(SystemType::Navigation);
        systemManager.RegisterUnifiedSystem(SystemType::GameplayEvent);
        systemManager.RegisterUnifiedSystem(SystemType::MissionScript);
    }

    // ================================
    // PLAYER SETUP - Designer-Friendly Configuration
    // ================================
    
#ifndef NDEBUG
    std::cout << "[Simulation] Creating player entity..." << std::endl;
#endif
    // Step 1: Create the core entity
    playerEntity = useEm->CreateEntity();
    std::cout.flush();
    
    // Step 2: Initialize Player actor (handles all component setup automatically)
    auto player = std::make_unique<Player>();
    ActorContext context{*useEm, playerEntity};
    player->AttachContext(context);
    player->Initialize();  // Player handles all its own component initialization
    
    std::cout << "[Simulation] Player actor initialized with all components" << std::endl;

    // Get PlayerPhysics component added by Player class (from designer configuration)
    if (auto* physics = useEm->GetComponent<PlayerPhysics>(playerEntity)) {
        CreatePlayerPhysicsComponents(*useEm, *physics);
    } else {
        std::cout << "[Simulation] Warning: No PlayerPhysics component found for RigidBody setup" << std::endl;
    }

    // Configure movement parameters and bounds for the game
    MovementBounds resolvedBounds = movementBoundsConfig;
    if (useMovementBoundsFile) {
        resolvedBounds = ResolveMovementBounds(movementBoundsConfig, movementBoundsConfigPath, movementBoundsProfile);
    }
    movementBoundsConfig = resolvedBounds;

    RebuildEnvironmentColliders(*useEm);

    MovementParameters resolvedParams = movementConfig;
    if (useMovementParametersFile) {
        resolvedParams = ResolveMovementParameters(movementConfig, movementParametersConfigPath, movementParametersProfile);
    }
    movementConfig = resolvedParams;
   
    std::cout << "[Simulation] ===== Player entity creation complete! Entity ID: " << playerEntity << " =====" << std::endl;

    // DEMONSTRATION: Auto-loading entity configuration system (disabled for faster startup)
    // Enable with environment variable: NOVA_DEMO_ENTITIES=1
    const char* demoEntitiesEnv = std::getenv("NOVA_DEMO_ENTITIES");
    if (demoEntitiesEnv && std::string(demoEntitiesEnv) == "1") {
        std::cout << "[Simulation] === EntityFactory Demonstration ===" << std::endl;
        EntityFactory factory(*useEm);
        
        // Show available entity types
        auto availableTypes = factory.GetAvailableTypes();
        std::cout << "[Simulation] Available entity types: ";
        for (size_t i = 0; i < availableTypes.size(); ++i) {
            std::cout << availableTypes[i];
            if (i < availableTypes.size() - 1) std::cout << ", ";
        }
        std::cout << std::endl;
        
        // Create a few example entities with auto-loaded configurations
        auto stationResult = factory.CreateStation("station", 100.0, 0.0, 50.0);
        if (stationResult.success) {
            std::cout << "[Simulation] Created station entity " << stationResult.entity << " with auto-loaded config" << std::endl;
        }
        
        auto traderResult = factory.CreateNPC("trader", -50.0, 0.0, 25.0);
        if (traderResult.success) {
            std::cout << "[Simulation] Created trader NPC entity " << traderResult.entity << " with auto-loaded config" << std::endl;
        }
        
        auto pirateResult = factory.CreateNPC("pirate", 75.0, 0.0, -30.0);
        if (pirateResult.success) {
            std::cout << "[Simulation] Created pirate NPC entity " << pirateResult.entity << " with auto-loaded config" << std::endl;
        }
        
        std::cout << "[Simulation] === End EntityFactory Demonstration ===" << std::endl;
    }

    inputRight = false;
    inputForward = false;
    inputBackward = false;
    inputUp = false;
    inputDown = false;
    inputStrafeLeft = false;
    inputStrafeRight = false;
    inputCameraYaw = 0.0;
    inputSprint = false;
    inputCrouch = false;
    inputSlide = false;
    inputBoost = false;
    prevJumpHeld = false;

    // Removed redundant debug message - detailed logging above

    if (useSchedulerV2_) {
        EnsureSchedulerV2Configured(*useEm);
    } else {
        schedulerV2_.Clear();
    }
}

void Simulation::Update(double dt) {
    // TODO: Advanced simulation update system
    // [ ] Update Scheduling: Intelligent scheduling of system updates based on priority
    // [ ] Load Balancing: Dynamic load balancing between different simulation systems
    // [ ] Performance Monitoring: Real-time monitoring of update performance
    // [ ] Adaptive Timesteps: Dynamic timestep adjustment based on simulation complexity
    // [ ] System Dependencies: Handle complex dependencies between system updates
    // [ ] Error Recovery: Graceful handling of errors during simulation updates
    // [ ] Memory Management: Efficient memory management during simulation updates
    // [ ] Threading Support: Multi-threaded simulation updates for better performance
    // [ ] State Validation: Validate simulation state consistency after updates
    // [ ] Profiler Integration: Deep integration with profiling tools for optimization
    
    if (dt <= 0.0) {
        return;
    }

    EntityManager* useEm = activeEm ? activeEm : &em;

    // Debug: Check player position every 2 seconds (DISABLED FOR PERFORMANCE)
    // static double posDebugTimer = 0.0;
    // posDebugTimer += dt;
    // if (posDebugTimer > 2.0) {
    //     if (auto* pos = useEm->GetComponent<Position>(playerEntity)) {
    //         std::cout << "[Simulation] Player position: (" << pos->x << ", " << pos->y << ", " << pos->z << ")" << std::endl;
    //     } else {
    //         std::cout << "[Simulation] WARNING: Player entity has no Position component!" << std::endl;
    //     }
    //     posDebugTimer = 0.0;
    // }

    // PERFORMANCE: Reduced auto-draw check frequency for better FPS
    // Auto-add DrawComponent to any entity with Position but no DrawComponent
    static int autoDrawCounter = 0;
    if (++autoDrawCounter % 360 == 0) {  // Check every 6 seconds instead of every second
        useEm->ForEach<Position>([useEm, this](Entity e, Position& pos) {
            // Skip player entity - it has custom cyan cube
            if (e == playerEntity) return;
            
            if (!useEm->GetComponent<DrawComponent>(e)) {
                auto draw = std::make_shared<DrawComponent>();
                draw->mode = DrawComponent::RenderMode::Mesh3D;
                draw->visible = true;
                draw->meshHandle = 0;
                draw->meshScale = 1.0f;
                draw->tintR = 0.8f;
                draw->tintG = 0.8f;
                draw->tintB = 0.8f;
                useEm->AddComponent<DrawComponent>(e, draw);
            }
        });
    }

    if (replayPlayer_.IsPlaying()) {
        if (const ReplayFrame* frame = replayPlayer_.ConsumeNextFrame()) {
            inputForward = frame->input.forward;
            inputBackward = frame->input.backward;
            inputUp = frame->input.up;
            inputDown = frame->input.down;
            inputStrafeLeft = frame->input.strafeLeft;
            inputStrafeRight = frame->input.strafeRight;
            inputSprint = frame->input.sprint;
            inputCrouch = frame->input.crouch;
            inputSlide = frame->input.slide;
            inputBoost = frame->input.boost;
            inputLeft = frame->input.left;
            inputRight = frame->input.right;
            inputCameraYaw = frame->input.cameraYaw;
            randomManager_.RestoreState(frame->randomState);
            replayPlayer_.ApplyFrameToEntities(*frame, *useEm);
        } else {
            replayPlayer_.StopPlayback();
        }
    }

    if (auto* controller = useEm->GetComponent<PlayerController>(playerEntity)) {
        bool jumpJustPressed = inputUp && !prevJumpHeld;
        
        // PERFORMANCE: Removed input debug logging from hot path
        
        controller->moveLeft = inputLeft;
        controller->moveRight = inputRight;
        controller->moveForward = inputForward;
        controller->moveBackward = inputBackward;
        controller->moveUp = useThrustMode ? inputUp : false;
        controller->moveDown = inputDown;
        controller->strafeLeft = inputStrafeLeft;
        controller->strafeRight = inputStrafeRight;
        controller->sprint = inputSprint;
        controller->crouch = inputCrouch;
        controller->slide = inputSlide;
        controller->boost = inputBoost;
        controller->cameraYaw = inputCameraYaw;
        controller->thrustMode = useThrustMode;
        controller->jumpRequested = (!useThrustMode && jumpJustPressed);
    }

    if (auto* physics = useEm->GetComponent<PlayerPhysics>(playerEntity)) {
        physics->thrustMode = useThrustMode;
    }

    // PERFORMANCE: Removed debug logging from hot path for better FPS
    if (useSchedulerV2_) {
        EnsureSchedulerV2Configured(*useEm);
        schedulerV2_.UpdateAll(useEm->GetArchetypeManager(), dt);
    } else {
        // Direct system calls for optimal performance
        useEm->EnableArchetypeFacade();
        if (playerControlSystem) playerControlSystem->Update(*useEm, dt);
        if (movementSystem) movementSystem->Update(*useEm, dt);
        if (locomotionSystem) locomotionSystem->Update(*useEm, dt);
    }

    // PERFORMANCE: Removed position debug logging
    if (auto* p = useEm->GetComponent<Position>(playerEntity)) {
        position = p->x;
    }

    prevJumpHeld = inputUp;

    elapsedTimeSeconds_ += dt;

    if (replayRecorder_.IsRecording()) {
        PlayerInputSnapshot snapshot;
        snapshot.forward = inputForward;
        snapshot.backward = inputBackward;
        snapshot.up = inputUp;
        snapshot.down = inputDown;
        snapshot.strafeLeft = inputStrafeLeft;
        snapshot.strafeRight = inputStrafeRight;
        snapshot.sprint = inputSprint;
        snapshot.crouch = inputCrouch;
        snapshot.slide = inputSlide;
        snapshot.boost = inputBoost;
        snapshot.left = inputLeft;
        snapshot.right = inputRight;
        snapshot.cameraYaw = inputCameraYaw;

        replayRecorder_.RecordFrame(elapsedTimeSeconds_, snapshot, randomManager_.GetState(), *useEm);
    }
}

double Simulation::GetPosition() const {
    return position;
}

double Simulation::GetPlayerX() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
    return p ? p->x : 0.0;
}

double Simulation::GetPlayerY() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
    return p ? p->y : 0.0;
}

double Simulation::GetPlayerZ() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    auto p = useEm->GetComponent<Position>(playerEntity);
    return p ? p->z : 0.0;
}

LocomotionStateMachine::State Simulation::GetLocomotionState() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    if (auto* locomotion = useEm->GetComponent<LocomotionStateMachine>(playerEntity)) {
        return locomotion->currentState;
    }
    return LocomotionStateMachine::State::Idle;
}

LocomotionStateMachine::Weights Simulation::GetLocomotionBlendWeights() const {
    const EntityManager* useEm = activeEm ? activeEm : &em;
    LocomotionStateMachine::Weights weights;
    if (auto* locomotion = useEm->GetComponent<LocomotionStateMachine>(playerEntity)) {
        weights = locomotion->blendWeights;
    }
    return weights;
}

std::shared_ptr<physics::IPhysicsEngine> Simulation::GetActivePhysicsEngine() const {
    // Note: Physics engine is now managed by UnifiedSystem
    // if (physicsSystem) {
    //     return physicsSystem->GetActiveEngine();
    // }
    return nullptr;
}

void Simulation::SetPlayerInput(bool forward, bool backward, bool up, bool down, bool strafeLeft, bool strafeRight, double cameraYaw,
                               bool sprint, bool crouch, bool slide, bool boost) {
    // TODO: Enhanced input processing system
    // [ ] Input Validation: Validate all input parameters for security and sanity
    // [ ] Input Buffering: Buffer input commands to prevent loss during frame drops
    // [ ] Input Smoothing: Smooth input transitions for better player experience
    // [ ] Input Recording: Record input sequences for replay and analysis
    // [ ] Input Prediction: Client-side prediction for networked gameplay
    // [ ] Input Remapping: Support for custom input remapping and profiles
    // [ ] Input Events: Event-driven input system for better modularity
    // [ ] Input Analytics: Collect analytics on input usage patterns
    // [ ] Input Security: Anti-cheat measures for input validation
    // [ ] Input Accessibility: Enhanced accessibility features for input handling
    
    inputForward = forward;
    inputBackward = backward;
    inputUp = up;
    inputDown = down;
    inputStrafeLeft = strafeLeft;
    inputStrafeRight = strafeRight;
    inputCameraYaw = cameraYaw;
    inputSprint = sprint;
    inputCrouch = crouch;
    inputSlide = slide;
    inputBoost = boost;
}

void Simulation::SetUseThrustMode(bool thrustMode) {
    useThrustMode = thrustMode;
    EntityManager* useEm = activeEm ? activeEm : &em;
    if (auto* physics = useEm->GetComponent<PlayerPhysics>(playerEntity)) {
        physics->thrustMode = thrustMode;
    }
    if (auto* controller = useEm->GetComponent<PlayerController>(playerEntity)) {
        controller->thrustMode = thrustMode;
    }
}

void Simulation::ConfigureMovementParameters(const MovementParameters& params) {
    movementConfig = params;
    useMovementParametersFile = false;
    EntityManager* useEm = activeEm ? activeEm : &em;
    if (!useEm->IsAlive(playerEntity)) {
        return;
    }

    if (auto* existing = useEm->GetComponent<MovementParameters>(playerEntity)) {
        *existing = movementConfig;
    } else {
        auto movementParams = std::make_shared<MovementParameters>(movementConfig);
        useEm->AddComponent<MovementParameters>(playerEntity, movementParams);
    }
}

void Simulation::SetMovementParametersConfigPath(const std::string& path) {
    movementParametersConfigPath = path;
    useMovementParametersFile = !movementParametersConfigPath.empty();
}

void Simulation::SetMovementParametersProfile(const std::string& profile) {
    movementParametersProfile = profile;
}

void Simulation::ConfigureMovementBounds(const MovementBounds& bounds) {
    // TODO: Advanced movement bounds system
    // [ ] Dynamic Bounds: Support for dynamically changing movement bounds
    // [ ] Bounds Validation: Validate movement bounds for consistency and safety
    // [ ] Bounds Events: Event system for movement bounds changes
    // [ ] Bounds Visualization: Debug visualization of movement bounds
    // [ ] Bounds Optimization: Optimize bounds checking for performance
    // [ ] Bounds Networking: Network synchronization of movement bounds
    // [ ] Bounds Scripting: Script-based movement bounds configuration
    // [ ] Bounds Analytics: Collect analytics on movement patterns within bounds
    // [ ] Bounds Security: Server-side validation of movement bounds
    // [ ] Bounds Accessibility: Accessibility features for movement bounds
    
    movementBoundsConfig = bounds;
    useMovementBoundsFile = false;

    EntityManager* useEm = activeEm ? activeEm : &em;
    if (!useEm->IsAlive(playerEntity)) {
        return;
    }

    RebuildEnvironmentColliders(*useEm);

    if (auto* existing = useEm->GetComponent<MovementBounds>(playerEntity)) {
        *existing = movementBoundsConfig;
    } else {
        auto movementBounds = std::make_shared<MovementBounds>(movementBoundsConfig);
        useEm->AddComponent<MovementBounds>(playerEntity, movementBounds);
    }
}

void Simulation::SetMovementBoundsConfigPath(const std::string& path) {
    movementBoundsConfigPath = path;
    useMovementBoundsFile = !movementBoundsConfigPath.empty();
}

void Simulation::SetMovementBoundsProfile(const std::string& profile) {
    movementBoundsProfile = profile;
}

void Simulation::StartReplayRecording(uint64_t seed) {
    // TODO: Advanced replay system
    // [ ] Compression: Compress replay data for storage efficiency
    // [ ] Streaming: Stream replay data for large simulations
    // [ ] Validation: Validate replay data integrity and consistency
    // [ ] Metadata: Rich metadata for replay files (timestamp, version, etc.)
    // [ ] Incremental Recording: Record only changes for efficiency
    // [ ] Multiple Tracks: Support for multiple replay tracks (input, state, events)
    // [ ] Replay Analysis: Tools for analyzing recorded replay data
    // [ ] Replay Editing: Edit and modify recorded replay sequences
    // [ ] Replay Sharing: Share replay files between users
    // [ ] Replay Security: Secure replay data against tampering
    
    if (seed != randomManager_.GetGlobalSeed()) {
        randomManager_.SetGlobalSeed(seed);
    }
    replayRecorder_.StartRecording(randomManager_.GetGlobalSeed());
    replayPlayer_.StopPlayback();
}

void Simulation::StopReplayRecording(const std::string& path) {
    if (!replayRecorder_.IsRecording()) {
        return;
    }
    replayRecorder_.StopRecording();
    if (!path.empty()) {
        replayRecorder_.SaveToFile(path);
    }
}

bool Simulation::LoadReplay(const std::string& path) {
    bool loaded = replayPlayer_.LoadFromFile(path);
    if (loaded) {
        replayRecorder_.StopRecording();
    }
    return loaded;
}

void Simulation::PlayLoadedReplay() {
    replayPlayer_.BeginPlayback();
}

void Simulation::StopReplayPlayback() {
    replayPlayer_.StopPlayback();
}

void Simulation::DestroyEnvironmentColliders(EntityManager& entityManager) {
    for (Entity colliderEntity : environmentColliderEntities) {
        if (entityManager.IsAlive(colliderEntity)) {
            entityManager.DestroyEntity(colliderEntity);
        }
    }
    environmentColliderEntities.clear();
}

void Simulation::RebuildEnvironmentColliders(EntityManager& entityManager) {
    DestroyEnvironmentColliders(entityManager);

    auto definitions = BuildEnvironmentFromBounds(movementBoundsConfig);
    environmentColliderEntities.reserve(definitions.size());

    for (const auto& def : definitions) {
        Entity colliderEntity = entityManager.CreateEntity();

        auto position = std::make_shared<Position>();
        position->x = def.centerX;
        position->y = def.centerY;
        position->z = def.centerZ;
        entityManager.AddComponent<Position>(colliderEntity, position);

        auto rigidBody = std::make_shared<RigidBody>();
        rigidBody->isKinematic = true;
        rigidBody->useGravity = false;
        rigidBody->linearDamping = 0.0;
        rigidBody->angularDamping = 0.0;
        rigidBody->UpdateInverseMass();
        entityManager.AddComponent<RigidBody>(colliderEntity, rigidBody);

        auto collider = std::make_shared<BoxCollider>();
        collider->width = def.sizeX;
        collider->height = def.sizeY;
        collider->depth = def.sizeZ;
        collider->collisionLayer = kCollisionLayerEnvironment;
        collider->collisionMask = kCollisionLayerPlayer;
        collider->isTrigger = false;
        entityManager.AddComponent<BoxCollider>(colliderEntity, collider);

        auto surface = std::make_shared<EnvironmentSurface>();
        surface->surfaceType = def.surfaceType;
        surface->overridesProfile = def.overridesProfile;
        surface->movementProfile = def.movementProfile;
        surface->isHazard = def.isHazard;
        surface->hazardModifier = def.hazardModifier;
        entityManager.AddComponent<EnvironmentSurface>(colliderEntity, surface);

        auto velocity = std::make_shared<Velocity>();
        velocity->vx = 0.0;
        velocity->vy = 0.0;
        velocity->vz = 0.0;
        entityManager.AddComponent<Velocity>(colliderEntity, velocity);

        environmentColliderEntities.push_back(colliderEntity);
    }

}

void Simulation::CreatePlayerPhysicsComponents(EntityManager& entityManager, PlayerPhysics& playerPhysics) {
    if (!entityManager.IsAlive(playerEntity)) {
        return;
    }

    auto rigidBody = std::make_shared<RigidBody>();
    rigidBody->SetMass(1.0);
    rigidBody->useGravity = playerPhysics.enableGravity;
    rigidBody->linearDamping = 0.0;
    rigidBody->angularDamping = 0.0;
    rigidBody->freezeRotationX = true;
    rigidBody->freezeRotationY = true;
    rigidBody->freezeRotationZ = true;
    entityManager.AddComponent<RigidBody>(playerEntity, rigidBody);

    auto collider = std::make_shared<BoxCollider>();
    collider->width = 1.0;
    collider->height = 1.0;
    collider->depth = 1.8;
    collider->offsetZ = collider->depth * 0.5;
    collider->collisionLayer = kCollisionLayerPlayer;
    collider->collisionMask = kCollisionLayerEnvironment;
    collider->isTrigger = false;
    entityManager.AddComponent<BoxCollider>(playerEntity, collider);

    if (!entityManager.GetComponent<CollisionInfo>(playerEntity)) {
        entityManager.EmplaceComponent<CollisionInfo>(playerEntity);
    }
}

void Simulation::EnsureSchedulerV2Configured(EntityManager& entityManager) {
    if (!useSchedulerV2_) {
        return;
    }

    if (!schedulerConfigured_) {
        ConfigureSchedulerV2(entityManager);
    }
}

void Simulation::ConfigureSchedulerV2(EntityManager& entityManager) {
    // TODO: Advanced scheduler configuration system
    // [ ] Dynamic Scheduling: Runtime modification of system scheduling
    // [ ] Priority Management: Priority-based system scheduling
    // [ ] Load Balancing: Automatic load balancing across available CPU cores
    // [ ] Dependency Management: Complex dependency handling between systems
    // [ ] Performance Monitoring: Real-time monitoring of scheduler performance
    // [ ] Scheduler Optimization: Automatic optimization of scheduling order
    // [ ] System Isolation: Sandboxed system execution for stability
    // [ ] Error Recovery: Graceful handling of system failures in scheduler
    // [ ] Scheduler Profiling: Detailed profiling of system execution times
    // [ ] Scheduler Debugging: Enhanced debugging tools for scheduler analysis
    
    entityManager.EnableArchetypeFacade();
    schedulerV2_.Clear();

    // Note: With unified system, individual system adapters are no longer needed
    // All systems are now handled by UnifiedSystem instances registered in Init()
    schedulerConfigured_ = true;
}


