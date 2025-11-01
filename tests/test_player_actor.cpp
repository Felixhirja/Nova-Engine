#include "../entities/Player.h"
#include "../engine/ActorContext.h"
#include "../engine/ecs/EntityManagerV2.h"
#include <iostream>

namespace {

Player CreateBoundPlayer(ecs::EntityManagerV2& em, ecs::EntityHandle entity) {
    ActorContext context;
    context.entityManager = &em;
    context.entity = entity;
    context.debugName = "player_test";
    Player player;
    player.AttachContext(context);
    player.BindEntity(entity);
    return player;
}

bool TestComponentCaching() {
    ecs::EntityManagerV2 em;
    ecs::EntityHandle entity = em.CreateEntity();
    em.AddComponent<Position>(entity);

    Player player = CreateBoundPlayer(em, entity);
    auto* first = player.PositionComponent();
    auto* second = player.PositionComponent();
    if (!first || first != second) {
        std::cerr << "Component cache returned mismatched pointers" << std::endl;
        return false;
    }

    em.DestroyEntity(entity);
    if (player.IsBound()) {
        std::cerr << "Player should unbind after entity destruction" << std::endl;
        return false;
    }

    ecs::EntityHandle nextEntity = em.CreateEntity();
    em.AddComponent<Position>(nextEntity);
    player.BindEntity(nextEntity);
    auto* rebound = player.PositionComponent();
    if (!rebound) {
        std::cerr << "Failed to resolve position component after rebind" << std::endl;
        return false;
    }
    rebound->x = 42.0;
    if (player.X() != 42.0) {
        std::cerr << "Player did not reflect updated component state after rebind" << std::endl;
        return false;
    }
    return true;
}

bool TestFacadeAccessorsAndProgression() {
    ecs::EntityManagerV2 em;
    ecs::EntityHandle entity = em.CreateEntity();

    em.AddComponent<Position>(entity);
    em.AddComponent<Velocity>(entity);
    em.AddComponent<MovementParameters>(entity);
    em.AddComponent<MovementBounds>(entity);
    em.AddComponent<PlayerController>(entity);
    em.AddComponent<PlayerPhysics>(entity);
    em.AddComponent<LocomotionStateMachine>(entity);
    em.AddComponent<TargetLock>(entity);
    em.AddComponent<PlayerInventory>(entity);
    em.AddComponent<PlayerProgression>(entity);
    em.AddComponent<PlayerVitals>(entity);
    em.AddComponent<DockingStatus>(entity);

    Player player = CreateBoundPlayer(em, entity);

    // Camera state
    auto* pos = player.PositionComponent();
    auto* controller = player.ControllerComponent();
    auto* physics = player.PhysicsComponent();
    auto* target = player.TargetLockComponent();
    if (!pos || !controller || !physics || !target) {
        std::cerr << "Failed to resolve baseline components" << std::endl;
        return false;
    }
    pos->x = 5.0;
    pos->y = 1.5;
    pos->z = -2.25;
    controller->facingYaw = 1.2;
    controller->cameraYaw = 0.75;
    physics->thrustMode = true;
    target->isLocked = true;
    target->offsetY = 6.0;

    auto view = player.GetCameraViewState();
    if (view.worldX != 5.0 || view.worldY != 1.5 || view.worldZ != -2.25) {
        std::cerr << "Camera view state did not mirror position" << std::endl;
        return false;
    }
    if (!view.isTargetLocked || !view.thrustMode) {
        std::cerr << "Camera view state missing lock/thrust data" << std::endl;
        return false;
    }

    // Progression & skill nodes
    player.AddExperience(5000.0);
    auto progressionState = player.GetProgressionState();
    if (progressionState.level <= 1) {
        std::cerr << "Experience did not increase player level" << std::endl;
        return false;
    }
    int skillBefore = progressionState.skillPoints;
    if (!player.UnlockSkillNode("pilot.vector_mastery")) {
        std::cerr << "Failed to unlock skill node with available points" << std::endl;
        return false;
    }
    if (player.UnlockSkillNode("pilot.vector_mastery")) {
        std::cerr << "Unlocking same node twice should fail" << std::endl;
        return false;
    }
    auto progressionAfter = player.GetProgressionState();
    if (progressionAfter.skillPoints != skillBefore - 1) {
        std::cerr << "Skill point count mismatch after unlock" << std::endl;
        return false;
    }

    // Inventory helpers
    Player::InventorySlot slot;
    slot.id = "prototype_sensor";
    slot.displayName = "Prototype Sensor";
    slot.massTons = 0.15;
    slot.volumeM3 = 0.02;
    slot.quantity = 1;
    if (!player.AddInventoryItem(slot)) {
        std::cerr << "Failed to add inventory slot" << std::endl;
        return false;
    }
    if (!player.RemoveInventoryItem("prototype_sensor", 1)) {
        std::cerr << "Failed to remove inventory slot" << std::endl;
        return false;
    }

    return true;
}

bool TestEventHooks() {
    ecs::EntityManagerV2 em;
    ecs::EntityHandle entity = em.CreateEntity();

    em.AddComponent<PlayerPhysics>(entity);
    em.AddComponent<LocomotionStateMachine>(entity);
    em.AddComponent<DockingStatus>(entity);
    em.AddComponent<PlayerVitals>(entity);
    em.AddComponent<Position>(entity);

    auto* physics = em.GetComponent<PlayerPhysics>(entity);
    auto* locomotion = em.GetComponent<LocomotionStateMachine>(entity);
    auto* docking = em.GetComponent<DockingStatus>(entity);
    auto* vitals = em.GetComponent<PlayerVitals>(entity);
    auto* pos = em.GetComponent<Position>(entity);

    physics->isGrounded = true;
    locomotion->currentState = LocomotionStateMachine::State::Idle;
    docking->isDocked = false;
    vitals->health = vitals->maxHealth;
    pos->x = 0.0;
    pos->y = 0.0;
    pos->z = 0.0;

    Player player = CreateBoundPlayer(em, entity);

    int jumpEvents = 0;
    int dockEvents = 0;
    int damageEvents = 0;

    player.OnJump([&](const Player::JumpEvent& evt) {
        ++jumpEvents;
        if (evt.x != pos->x) {
            std::cerr << "Jump event did not snapshot world position" << std::endl;
        }
    });
    player.OnDock([&](const Player::DockEvent&) { ++dockEvents; });
    player.OnDamageTaken([&](const Player::DamageEvent&) { ++damageEvents; });

    player.PumpEvents(0.0); // prime state

    locomotion->currentState = LocomotionStateMachine::State::Airborne;
    physics->isGrounded = false;
    player.PumpEvents(0.1);
    if (jumpEvents != 1) {
        std::cerr << "Expected jump event after leaving ground" << std::endl;
        return false;
    }

    docking->isDocked = true;
    docking->portId = "alpha";
    player.PumpEvents(0.2);
    if (dockEvents != 1) {
        std::cerr << "Expected dock event when flag toggled" << std::endl;
        return false;
    }

    vitals->health -= 10.0;
    player.PumpEvents(0.3);
    if (damageEvents != 1) {
        std::cerr << "Expected damage event when health dropped" << std::endl;
        return false;
    }

    return true;
}

} // namespace

int main() {
    if (!TestComponentCaching()) {
        return 1;
    }
    if (!TestFacadeAccessorsAndProgression()) {
        return 2;
    }
    if (!TestEventHooks()) {
        return 3;
    }

    std::cout << "Player actor tests passed" << std::endl;
    return 0;
}
