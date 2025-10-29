#include "Player.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace {
constexpr double kEpsilon = 1e-6;
}

Player::Player() {
    boundEntity_ = ecs::EntityHandle::Null();
}

std::string_view Player::TypeName() const {
    return "Player";
}

bool Player::EnsureEntityAlive() const {
    if (!context_.entityManager) {
        return false;
    }
    if (!context_.entity.IsValid()) {
        return false;
    }
    if (!context_.entityManager->IsAlive(context_.entity)) {
        const_cast<Player*>(this)->ResetBinding();
        return false;
    }
    return true;
}

void Player::ResetBinding() {
    context_.ResetEntity();
    boundEntity_ = ecs::EntityHandle::Null();
    eventState_ = EventState{};
    elapsedSeconds_ = 0.0;
}

bool Player::IsBound() const {
    return EnsureEntityAlive();
}

ecs::EntityHandle Player::GetEntity() const {
    if (!EnsureEntityAlive()) {
        return ecs::EntityHandle::Null();
    }
    return context_.entity;
}

void Player::AttachContext(const ActorContext& context) {
    context_ = context;
    boundEntity_ = context.entity;
    eventState_ = EventState{};
    elapsedSeconds_ = 0.0;
}

const ActorContext& Player::Context() const {
    return context_;
}

void Player::BindEntity(ecs::EntityHandle entity) {
    if (!context_.entityManager) {
        boundEntity_ = ecs::EntityHandle::Null();
        context_.ResetEntity();
        eventState_ = EventState{};
        elapsedSeconds_ = 0.0;
        return;
    }

    context_.entity = entity;
    boundEntity_ = entity;
    eventState_ = EventState{};
    elapsedSeconds_ = 0.0;
}

Position* Player::PositionComponent() const {
    return ResolveComponent<Position>();
}

Velocity* Player::VelocityComponent() const {
    return ResolveComponent<Velocity>();
}

PlayerController* Player::ControllerComponent() const {
    return ResolveComponent<PlayerController>();
}

PlayerPhysics* Player::PhysicsComponent() const {
    return ResolveComponent<PlayerPhysics>();
}

TargetLock* Player::TargetLockComponent() const {
    return ResolveComponent<TargetLock>();
}

MovementParameters* Player::MovementParametersComponent() const {
    return ResolveComponent<MovementParameters>();
}

MovementBounds* Player::MovementBoundsComponent() const {
    return ResolveComponent<MovementBounds>();
}

LocomotionStateMachine* Player::LocomotionComponent() const {
    return ResolveComponent<LocomotionStateMachine>();
}

PlayerInventory* Player::InventoryComponent() const {
    return ResolveComponent<PlayerInventory>();
}

PlayerProgression* Player::ProgressionComponent() const {
    return ResolveComponent<PlayerProgression>();
}

PlayerVitals* Player::VitalsComponent() const {
    return ResolveComponent<PlayerVitals>();
}

DockingStatus* Player::DockingComponent() const {
    return ResolveComponent<DockingStatus>();
}

double Player::X() const {
    if (auto* pos = PositionComponent()) {
        return pos->x;
    }
    return 0.0;
}

double Player::Y() const {
    if (auto* pos = PositionComponent()) {
        return pos->y;
    }
    return 0.0;
}

double Player::Z() const {
    if (auto* pos = PositionComponent()) {
        return pos->z;
    }
    return 0.0;
}

Player::CameraViewState Player::GetCameraViewState() const {
    CameraViewState state;
    if (auto* pos = PositionComponent()) {
        state.worldX = pos->x;
        state.worldY = pos->y;
        state.worldZ = pos->z;
    }
    if (auto* controller = ControllerComponent()) {
        state.facingYaw = controller->facingYaw;
        state.cameraYaw = controller->cameraYaw;
    }
    if (auto* physics = PhysicsComponent()) {
        state.thrustMode = physics->thrustMode;
    }
    if (auto* target = TargetLockComponent()) {
        state.isTargetLocked = target->isLocked;
        state.targetOffsetY = target->offsetY;
    }
    return state;
}

void Player::AddExperience(double amount) {
    if (amount <= 0.0) {
        return;
    }
    auto* progression = ProgressionComponent();
    if (!progression) {
        return;
    }

    progression->experience += amount;
    progression->lifetimeExperience += amount;

    bool leveled = false;
    while (progression->experience + kEpsilon >= ExperienceForNextLevel(progression->level)) {
        double requirement = ExperienceForNextLevel(progression->level);
        if (requirement <= 0.0) {
            break;
        }
        progression->experience -= requirement;
        ++progression->level;
        ++progression->skillPoints;
        leveled = true;
    }

    if (!leveled) {
        // Keep experience within non-negative bounds if no level up occurred
        progression->experience = std::max(0.0, progression->experience);
    }
}

Player::ProgressionState Player::GetProgressionState() const {
    ProgressionState state;
    if (auto* progression = ProgressionComponent()) {
        state.level = progression->level;
        state.skillPoints = progression->skillPoints;
        state.experience = progression->experience;
        state.lifetimeExperience = progression->lifetimeExperience;
        state.unlockedSkillNodes = progression->unlockedSkillNodes;
    }
    return state;
}

bool Player::UnlockSkillNode(const std::string& nodeId) {
    auto* progression = ProgressionComponent();
    if (!progression) {
        return false;
    }
    if (nodeId.empty()) {
        return false;
    }
    if (progression->unlockedSkillNodes.find(nodeId) != progression->unlockedSkillNodes.end()) {
        return false;
    }
    if (progression->skillPoints <= 0) {
        return false;
    }
    progression->unlockedSkillNodes.insert(nodeId);
    --progression->skillPoints;
    return true;
}

bool Player::AddInventoryItem(const InventorySlot& slot) {
    if (slot.id.empty() || slot.quantity <= 0) {
        return false;
    }
    auto* inventory = InventoryComponent();
    if (!inventory) {
        return false;
    }

    double massDelta = slot.massTons * static_cast<double>(slot.quantity);
    double volumeDelta = slot.volumeM3 * static_cast<double>(slot.quantity);

    if (inventory->carriedMassTons + massDelta - kEpsilon > inventory->maxMassTons) {
        return false;
    }
    if (inventory->carriedVolumeM3 + volumeDelta - kEpsilon > inventory->maxVolumeM3) {
        return false;
    }

    auto it = std::find_if(inventory->items.begin(), inventory->items.end(), [&](const PlayerInventory::ItemSlot& existing) {
        return existing.id == slot.id;
    });

    if (it != inventory->items.end()) {
        it->quantity += slot.quantity;
        it->massTons = slot.massTons;
        it->volumeM3 = slot.volumeM3;
        it->displayName = slot.displayName.empty() ? it->displayName : slot.displayName;
        it->equipped = slot.equipped;
        it->questItem = slot.questItem;
    } else {
        PlayerInventory::ItemSlot newSlot;
        newSlot.id = slot.id;
        newSlot.displayName = slot.displayName;
        newSlot.massTons = slot.massTons;
        newSlot.volumeM3 = slot.volumeM3;
        newSlot.quantity = slot.quantity;
        newSlot.equipped = slot.equipped;
        newSlot.questItem = slot.questItem;
        inventory->items.push_back(std::move(newSlot));
    }

    inventory->carriedMassTons += massDelta;
    inventory->carriedVolumeM3 += volumeDelta;
    return true;
}

bool Player::RemoveInventoryItem(const std::string& id, int quantity) {
    if (id.empty() || quantity <= 0) {
        return false;
    }
    auto* inventory = InventoryComponent();
    if (!inventory) {
        return false;
    }

    auto it = std::find_if(inventory->items.begin(), inventory->items.end(), [&](const PlayerInventory::ItemSlot& existing) {
        return existing.id == id;
    });

    if (it == inventory->items.end()) {
        return false;
    }
    if (quantity > it->quantity) {
        return false;
    }

    double massDelta = it->massTons * static_cast<double>(quantity);
    double volumeDelta = it->volumeM3 * static_cast<double>(quantity);

    it->quantity -= quantity;
    if (it->quantity == 0) {
        inventory->items.erase(it);
    }

    inventory->carriedMassTons = std::max(0.0, inventory->carriedMassTons - massDelta);
    inventory->carriedVolumeM3 = std::max(0.0, inventory->carriedVolumeM3 - volumeDelta);
    return true;
}

void Player::OnJump(JumpCallback callback) {
    if (callback) {
        jumpCallbacks_.push_back(std::move(callback));
    }
}

void Player::OnDock(DockCallback callback) {
    if (callback) {
        dockCallbacks_.push_back(std::move(callback));
    }
}

void Player::OnDamageTaken(DamageCallback callback) {
    if (callback) {
        damageCallbacks_.push_back(std::move(callback));
    }
}

void Player::PumpEvents(double deltaSeconds) {
    if (!EnsureEntityAlive()) {
        eventState_ = EventState{};
        elapsedSeconds_ = 0.0;
        return;
    }

    elapsedSeconds_ += deltaSeconds;

    auto* physics = PhysicsComponent();
    auto* locomotion = LocomotionComponent();
    auto* docking = DockingComponent();
    auto* vitals = VitalsComponent();
    auto* position = PositionComponent();

    bool grounded = physics ? physics->isGrounded : true;
    bool docked = docking ? docking->isDocked : false;

    if (!eventState_.initialized) {
        eventState_.initialized = true;
        eventState_.lastGrounded = grounded;
        eventState_.lastDocked = docked;
        if (vitals) {
            eventState_.lastHealth = vitals->health;
            eventState_.hasHealth = true;
        } else {
            eventState_.lastHealth = 0.0;
            eventState_.hasHealth = false;
        }
        return;
    }

    if (eventState_.lastGrounded && !grounded) {
        JumpEvent evt;
        if (position) {
            evt.x = position->x;
            evt.y = position->y;
            evt.z = position->z;
        }
        evt.time = elapsedSeconds_;
        for (const auto& cb : jumpCallbacks_) {
            cb(evt);
        }
    }

    eventState_.lastGrounded = grounded;

    if (!eventState_.lastDocked && docked) {
        DockEvent evt;
        evt.time = elapsedSeconds_;
        if (docking) {
            evt.portId = docking->portId;
        }
        for (const auto& cb : dockCallbacks_) {
            cb(evt);
        }
    }

    eventState_.lastDocked = docked;

    if (vitals) {
        if (!eventState_.hasHealth) {
            eventState_.lastHealth = vitals->health;
            eventState_.hasHealth = true;
        } else if (vitals->health + kEpsilon < eventState_.lastHealth) {
            DamageEvent evt;
            evt.amount = eventState_.lastHealth - vitals->health;
            evt.currentHealth = vitals->health;
            evt.time = elapsedSeconds_;
            for (const auto& cb : damageCallbacks_) {
                cb(evt);
            }
            eventState_.lastHealth = vitals->health;
        } else {
            eventState_.lastHealth = vitals->health;
        }
    } else {
        eventState_.hasHealth = false;
        eventState_.lastHealth = 0.0;
    }

    if (locomotion) {
        locomotion->previousState = locomotion->currentState;
    }
}

double Player::ExperienceForNextLevel(int level) {
    return std::max(1000.0, 1000.0 * static_cast<double>(level));
}

