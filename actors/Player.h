#pragma once

#include "Actor.h"
#include "ecs/Components.h"

#include <functional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

class Player : public IActor {
public:
    struct CameraViewState {
        double worldX = 0.0;
        double worldY = 0.0;
        double worldZ = 0.0;
        double facingYaw = 0.0;
        double cameraYaw = camera_defaults::kDefaultYawRadians;
        bool thrustMode = false;
        bool isTargetLocked = false;
        double targetOffsetY = 0.0;
    };

    struct ProgressionState {
        int level = 1;
        int skillPoints = 0;
        double experience = 0.0;
        double lifetimeExperience = 0.0;
        std::unordered_set<std::string> unlockedSkillNodes;
    };

    struct InventorySlot {
        std::string id;
        std::string displayName;
        double massTons = 0.0;
        double volumeM3 = 0.0;
        int quantity = 0;
        bool equipped = false;
        bool questItem = false;
    };

    struct JumpEvent {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
        double time = 0.0;
    };

    struct DockEvent {
        std::string portId;
        double time = 0.0;
    };

    struct DamageEvent {
        double amount = 0.0;
        double currentHealth = 0.0;
        double time = 0.0;
    };

    using JumpCallback = std::function<void(const JumpEvent&)>;
    using DockCallback = std::function<void(const DockEvent&)>;
    using DamageCallback = std::function<void(const DamageEvent&)>;

    Player();

    std::string_view TypeName() const override;
    bool IsBound() const override;
    ecs::EntityHandle GetEntity() const override;
    void AttachContext(const ActorContext& context) override;
    const ActorContext& Context() const override;

    void BindEntity(ecs::EntityHandle entity);

    Position* PositionComponent() const;
    Velocity* VelocityComponent() const;
    PlayerController* ControllerComponent() const;
    PlayerPhysics* PhysicsComponent() const;
    TargetLock* TargetLockComponent() const;
    MovementParameters* MovementParametersComponent() const;
    MovementBounds* MovementBoundsComponent() const;
    LocomotionStateMachine* LocomotionComponent() const;
    PlayerInventory* InventoryComponent() const;
    PlayerProgression* ProgressionComponent() const;
    PlayerVitals* VitalsComponent() const;
    DockingStatus* DockingComponent() const;

    double X() const;
    double Y() const;
    double Z() const;

    CameraViewState GetCameraViewState() const;

    void AddExperience(double amount);
    ProgressionState GetProgressionState() const;
    bool UnlockSkillNode(const std::string& nodeId);

    bool AddInventoryItem(const InventorySlot& slot);
    bool RemoveInventoryItem(const std::string& id, int quantity);

    void OnJump(JumpCallback callback);
    void OnDock(DockCallback callback);
    void OnDamageTaken(DamageCallback callback);
    void PumpEvents(double deltaSeconds);

    bool ToggleTargetLock();
    bool IsTargetLocked() const;

private:
    bool EnsureEntityAlive() const;
    void ResetBinding();

    template<typename T>
    T* ResolveComponent() const {
        if (!EnsureEntityAlive()) {
            return nullptr;
        }
        return context_.GetComponent<T>();
    }

    static double ExperienceForNextLevel(int level);

    ActorContext context_{};
    ecs::EntityHandle boundEntity_{};

    struct EventState {
        bool initialized = false;
        bool lastGrounded = true;
        bool lastDocked = false;
        double lastHealth = 0.0;
        bool hasHealth = false;
    };

    EventState eventState_{};
    double elapsedSeconds_ = 0.0;

    std::vector<JumpCallback> jumpCallbacks_;
    std::vector<DockCallback> dockCallbacks_;
    std::vector<DamageCallback> damageCallbacks_;
};

