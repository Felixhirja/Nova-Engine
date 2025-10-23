#ifndef SIMULATION_H
#define SIMULATION_H

#include "ecs/Components.h"
#include "ecs/EntityManager.h"
#include "ecs/System.h"
#include "ecs/SystemSchedulerV2.h"

#include <string>
#include <vector>

class PhysicsSystem;

class Simulation {
public:
    Simulation();
    ~Simulation();

    void Init(EntityManager* em = nullptr);
    // Update the simulation by dt seconds
    void Update(double dt);
    // Simple debug: get position
    double GetPosition() const;
    double GetPlayerX() const;
    double GetPlayerY() const;
    double GetPlayerZ() const;
    LocomotionStateMachine::State GetLocomotionState() const;
    LocomotionStateMachine::Weights GetLocomotionBlendWeights() const;
    Entity GetPlayerEntity() const { return playerEntity; }
    // Set player input state
    void SetPlayerInput(bool forward, bool backward, bool up, bool down, bool strafeLeft, bool strafeRight, double cameraYaw);
    void SetUseThrustMode(bool thrustMode);
    void ConfigureMovementParameters(const MovementParameters& params);
    const MovementParameters& GetMovementParameters() const { return movementConfig; }
    void SetUseSchedulerV2(bool enabled);
    bool IsUsingSchedulerV2() const { return useSchedulerV2_; }
    void SetMovementParametersConfigPath(const std::string& path);
    void SetMovementParametersProfile(const std::string& profile);
    const std::string& GetMovementParametersConfigPath() const { return movementParametersConfigPath; }
    const std::string& GetMovementParametersProfile() const { return movementParametersProfile; }
    void ConfigureMovementBounds(const MovementBounds& bounds);
    const MovementBounds& GetMovementBounds() const { return movementBoundsConfig; }
    void SetMovementBoundsConfigPath(const std::string& path);
    void SetMovementBoundsProfile(const std::string& profile);
    const std::string& GetMovementBoundsConfigPath() const { return movementBoundsConfigPath; }
    const std::string& GetMovementBoundsProfile() const { return movementBoundsProfile; }

private:
    // Basic simulation state implemented with ECS
    EntityManager em;
    EntityManager* activeEm = nullptr;
    Entity playerEntity = 0;
    // fallback scalar for global sim position if needed
    double position = 0.0;
    bool inputForward;
    bool inputBackward;
    bool inputUp;
    bool inputDown;
    bool inputStrafeLeft;
    bool inputStrafeRight;
    double inputCameraYaw;
    bool prevJumpHeld;
    bool useThrustMode;
    bool inputLeft;
    bool inputRight;
    SystemManager systemManager;
    ecs::SystemSchedulerV2 schedulerV2_;
    bool useSchedulerV2_ = false;
    bool schedulerConfigured_ = false;
    MovementParameters movementConfig;
    MovementBounds movementBoundsConfig;
    std::string movementParametersConfigPath;
    std::string movementParametersProfile;
    bool useMovementParametersFile;
    std::string movementBoundsConfigPath;
    std::string movementBoundsProfile;
    bool useMovementBoundsFile;

    PhysicsSystem* physicsSystem = nullptr;
    std::vector<Entity> environmentColliderEntities;

    void DestroyEnvironmentColliders(EntityManager& entityManager);
    void RebuildEnvironmentColliders(EntityManager& entityManager);
    void CreatePlayerPhysicsComponents(EntityManager& entityManager, PlayerPhysics& playerPhysics);
    void EnsureSchedulerV2Configured(EntityManager& entityManager);
    void ConfigureSchedulerV2(EntityManager& entityManager);
};

#endif // SIMULATION_H
