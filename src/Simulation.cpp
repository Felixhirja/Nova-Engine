#include "Simulation.h"
#include "ecs/AnimationSystem.h"
#include "ecs/LegacySystemAdapter.h"
#include "ecs/LocomotionSystem.h"
#include "ecs/MovementSystem.h"
#include "ecs/PhysicsSystem.h"
#include "ecs/PlayerControlSystem.h"
#include "ecs/SpaceshipPhysicsSystem.h"
#include "TargetingSystem.h"
#include "WeaponSystem.h"
#include "ShieldSystem.h"

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

std::string Trim(const std::string& value) {
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

bool ParseBool(const std::string& rawValue, bool& outValue) {
    std::string value = Trim(rawValue);
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

bool ParseDouble(const std::string& rawValue, double& outValue) {
    std::string value = Trim(rawValue);
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
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (inProfile) {
                commitProfile();
            }
            currentProfile = Trim(trimmed.substr(1, trimmed.size() - 2));
            currentBounds = MovementBounds();
            inProfile = true;
            continue;
        }

        auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos || !inProfile) {
            continue;
        }

        std::string key = Trim(trimmed.substr(0, equalsPos));
        std::string value = Trim(trimmed.substr(equalsPos + 1));

        if (key.empty()) {
            continue;
        }

        double numericValue = 0.0;
        bool boolValue = false;
        if (key == "minX") {
            if (ParseDouble(value, numericValue)) {
                currentBounds.minX = numericValue;
            }
        } else if (key == "maxX") {
            if (ParseDouble(value, numericValue)) {
                currentBounds.maxX = numericValue;
            }
        } else if (key == "minY") {
            if (ParseDouble(value, numericValue)) {
                currentBounds.minY = numericValue;
            }
        } else if (key == "maxY") {
            if (ParseDouble(value, numericValue)) {
                currentBounds.maxY = numericValue;
            }
        } else if (key == "minZ") {
            if (ParseDouble(value, numericValue)) {
                currentBounds.minZ = numericValue;
            }
        } else if (key == "maxZ") {
            if (ParseDouble(value, numericValue)) {
                currentBounds.maxZ = numericValue;
            }
        } else if (key == "clampX") {
            if (ParseBool(value, boolValue)) {
                currentBounds.clampX = boolValue;
            }
        } else if (key == "clampY") {
            if (ParseBool(value, boolValue)) {
                currentBounds.clampY = boolValue;
            }
        } else if (key == "clampZ") {
            if (ParseBool(value, boolValue)) {
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
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (inProfile) {
                commitProfile();
            }
            currentProfile = Trim(trimmed.substr(1, trimmed.size() - 2));
            currentParams = MovementParameters();
            inProfile = true;
            continue;
        }

        auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos || !inProfile) {
            continue;
        }

        std::string key = Trim(trimmed.substr(0, equalsPos));
        std::string value = Trim(trimmed.substr(equalsPos + 1));

        if (key.empty()) {
            continue;
        }

        double numericValue = 0.0;
        if (ParseDouble(value, numericValue)) {
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

bool IsRelativePath(const std::string& path) {
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
    if (IsRelativePath(path)) {
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
    if (IsRelativePath(path)) {
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
      useMovementBoundsFile(true) {
    activeEm = &em;
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
    position = 0.0;
    std::cout << "Simulation initialized. position=" << position << std::endl;

    activeEm = externalEm ? externalEm : &em;
    EntityManager* useEm = activeEm;

    schedulerConfigured_ = false;

    DestroyEnvironmentColliders(*useEm);

    if (!externalEm) {
        useEm->Clear();
    }

    systemManager.Clear();
    systemManager.RegisterSystem<PlayerControlSystem>();
    systemManager.RegisterSystem<SpaceshipPhysicsSystem>();
    systemManager.RegisterSystem<MovementSystem>();
    systemManager.RegisterSystem<LocomotionSystem>();
    systemManager.RegisterSystem<AnimationSystem>();
    systemManager.RegisterSystem<TargetingSystem>();
    systemManager.RegisterSystem<WeaponSystem>();
    systemManager.RegisterSystem<ShieldSystem>();

    // Create player entity in ECS
    playerEntity = useEm->CreateEntity();
    auto pos = std::make_shared<Position>();
    pos->x = 0.0;
    pos->y = 0.0;
    useEm->AddComponent<Position>(playerEntity, pos);

    auto vel = std::make_shared<Velocity>();
    vel->vx = 0.0;
    vel->vy = 0.0;
    useEm->AddComponent<Velocity>(playerEntity, vel);

    auto acc = std::make_shared<Acceleration>();
    acc->ax = 0.0;
    acc->ay = 0.0;
    acc->az = 0.0;
    useEm->AddComponent<Acceleration>(playerEntity, acc);

    auto controller = std::make_shared<PlayerController>();
    controller->moveLeft = false;
    controller->moveRight = false;
    controller->moveForward = false;
    controller->moveBackward = false;
    controller->moveUp = false;
    controller->moveDown = false;
    controller->strafeLeft = false;
    controller->strafeRight = false;
    controller->sprint = false;
    controller->crouch = false;
    controller->slide = false;
    controller->boost = false;
    controller->cameraYaw = 0.0;
    useEm->AddComponent<PlayerController>(playerEntity, controller);

    auto physics = std::make_shared<PlayerPhysics>();
    physics->thrustMode = useThrustMode;
    physics->enableGravity = true;
    physics->isGrounded = true;
    useEm->AddComponent<PlayerPhysics>(playerEntity, physics);

    CreatePlayerPhysicsComponents(*useEm, *physics);

    if (physicsSystem) {
        physicsSystem->SetGravity(0.0, 0.0, physics->gravity);
    }

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

    auto movementParams = std::make_shared<MovementParameters>(movementConfig);
    useEm->AddComponent<MovementParameters>(playerEntity, movementParams);

    auto locomotion = std::make_shared<LocomotionStateMachine>();
    locomotion->wasGrounded = physics->isGrounded;
    const double forwardMax = std::max(0.0, movementConfig.forwardMaxSpeed);
    const double backwardMax = std::max(0.0, movementConfig.backwardMaxSpeed);
    const double strafeMax = std::max(0.0, movementConfig.strafeMaxSpeed);
    const double baseSpeed = std::max(forwardMax, std::max(backwardMax, strafeMax));
    if (baseSpeed > 0.0) {
        locomotion->idleSpeedThreshold = std::max(0.1, baseSpeed * 0.1);
        locomotion->walkSpeedThreshold = std::max(locomotion->idleSpeedThreshold + 0.1, baseSpeed * 0.4);
        locomotion->sprintSpeedThreshold = std::max(locomotion->walkSpeedThreshold + 0.1, baseSpeed * 0.85);
        locomotion->slideSpeedThreshold = std::max(locomotion->walkSpeedThreshold, baseSpeed * 0.65);
    }
    locomotion->stamina = locomotion->maxStamina;
    locomotion->heat = 0.0;
    locomotion->activeSurfaceType = locomotion->defaultSurfaceType;
    if (locomotion->surfaceProfiles.count(locomotion->defaultSurfaceType)) {
        locomotion->activeSurfaceProfile = locomotion->surfaceProfiles.at(locomotion->defaultSurfaceType);
    }
    locomotion->activeHazardModifier = locomotion->hazardBaseline;
    locomotion->currentCameraOffset = locomotion->defaultCameraOffset;
    locomotion->baseJumpImpulse = physics->jumpImpulse;
    useEm->AddComponent<LocomotionStateMachine>(playerEntity, locomotion);

    auto targetLock = std::make_shared<TargetLock>();
    targetLock->targetEntityId = 0;  // No target initially
    targetLock->isLocked = false;    // Start unlocked
    targetLock->offsetX = 0.0;
    targetLock->offsetY = 5.0;
    targetLock->offsetZ = 10.0;
    targetLock->followDistance = 15.0;
    targetLock->followHeight = 5.0;
    useEm->AddComponent<TargetLock>(playerEntity, targetLock);
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

    std::cout << "Simulation: created player entity id=" << playerEntity << std::endl;

    if (useSchedulerV2_) {
        EnsureSchedulerV2Configured(*useEm);
    } else {
        schedulerV2_.Clear();
    }
}

void Simulation::Update(double dt) {
    if (dt <= 0.0) {
        return;
    }

    EntityManager* useEm = activeEm ? activeEm : &em;

    if (auto* controller = useEm->GetComponent<PlayerController>(playerEntity)) {
        bool jumpJustPressed = inputUp && !prevJumpHeld;
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

    if (useSchedulerV2_) {
        EnsureSchedulerV2Configured(*useEm);
        schedulerV2_.UpdateAll(useEm->GetArchetypeManager(), dt);
    } else {
        systemManager.UpdateAll(*useEm, dt);
    }

    if (auto* p = useEm->GetComponent<Position>(playerEntity)) {
        position = p->x;
    }

    prevJumpHeld = inputUp;
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
    if (physicsSystem) {
        return physicsSystem->GetActiveEngine();
    }
    return nullptr;
}

void Simulation::SetPlayerInput(bool forward, bool backward, bool up, bool down, bool strafeLeft, bool strafeRight, double cameraYaw,
                               bool sprint, bool crouch, bool slide, bool boost) {
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
    movementBoundsConfig = bounds;
    useMovementBoundsFile = false;

    EntityManager* useEm = activeEm ? activeEm : &em;
    if (!useEm->IsAlive(playerEntity)) {
        return;
    }

    RebuildEnvironmentColliders(*useEm);
}

void Simulation::SetMovementBoundsConfigPath(const std::string& path) {
    movementBoundsConfigPath = path;
    useMovementBoundsFile = !movementBoundsConfigPath.empty();
}

void Simulation::SetMovementBoundsProfile(const std::string& profile) {
    movementBoundsProfile = profile;
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
    entityManager.EnableArchetypeFacade();
    schedulerV2_.Clear();

    using PlayerAdapter = ecs::LegacySystemAdapter<PlayerControlSystem>;
    using SpaceshipAdapter = ecs::LegacySystemAdapter<SpaceshipPhysicsSystem>;
    using MovementAdapter = ecs::LegacySystemAdapter<MovementSystem>;
    using LocomotionAdapter = ecs::LegacySystemAdapter<LocomotionSystem>;
    using AnimationAdapter = ecs::LegacySystemAdapter<AnimationSystem>;
    using TargetingAdapter = ecs::LegacySystemAdapter<TargetingSystem>;
    using WeaponAdapter = ecs::LegacySystemAdapter<WeaponSystem>;
    using ShieldAdapter = ecs::LegacySystemAdapter<ShieldSystem>;

    ecs::LegacySystemAdapterConfig playerConfig;
    playerConfig.phase = ecs::UpdatePhase::Input;
    schedulerV2_.RegisterSystem<PlayerAdapter>(entityManager, playerConfig);

    ecs::LegacySystemAdapterConfig spaceshipConfig;
    spaceshipConfig.phase = ecs::UpdatePhase::Input;
    spaceshipConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<PlayerAdapter>());
    schedulerV2_.RegisterSystem<SpaceshipAdapter>(entityManager, spaceshipConfig);

    ecs::LegacySystemAdapterConfig movementConfig;
    movementConfig.phase = ecs::UpdatePhase::Simulation;
    movementConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<SpaceshipAdapter>());
    schedulerV2_.RegisterSystem<MovementAdapter>(entityManager, movementConfig);

    ecs::LegacySystemAdapterConfig locomotionConfig;
    locomotionConfig.phase = ecs::UpdatePhase::Simulation;
    locomotionConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<MovementAdapter>());
    schedulerV2_.RegisterSystem<LocomotionAdapter>(entityManager, locomotionConfig);

    ecs::LegacySystemAdapterConfig animationConfig;
    animationConfig.phase = ecs::UpdatePhase::Simulation;
    animationConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<LocomotionAdapter>());
    schedulerV2_.RegisterSystem<AnimationAdapter>(entityManager, animationConfig);

    ecs::LegacySystemAdapterConfig targetingConfig;
    targetingConfig.phase = ecs::UpdatePhase::Simulation;
    targetingConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<AnimationAdapter>());
    schedulerV2_.RegisterSystem<TargetingAdapter>(entityManager, targetingConfig);

    ecs::LegacySystemAdapterConfig weaponConfig;
    weaponConfig.phase = ecs::UpdatePhase::RenderPrep;
    weaponConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<TargetingAdapter>());
    schedulerV2_.RegisterSystem<WeaponAdapter>(entityManager, weaponConfig);

    ecs::LegacySystemAdapterConfig shieldConfig;
    shieldConfig.phase = ecs::UpdatePhase::RenderPrep;
    shieldConfig.systemDependencies.push_back(ecs::SystemDependency::Requires<WeaponAdapter>());
    schedulerV2_.RegisterSystem<ShieldAdapter>(entityManager, shieldConfig);

    schedulerConfigured_ = true;
}


