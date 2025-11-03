#include "BulletPhysicsEngine.h"

#include <algorithm>

#include "../ecs/System.h"

namespace physics {

BulletPhysicsEngine::BulletPhysicsEngine() {
    params_.debugName = "BulletCompatibility";
}

void BulletPhysicsEngine::Initialize(const PhysicsEngineInitParams& params) {
    params_ = params;
    if (params_.fixedTimeStep <= 0.0) {
        params_.fixedTimeStep = 1.0 / 60.0;
    }
    if (params_.maxSubSteps == 0) {
        params_.maxSubSteps = 60;
    }
    if (params_.debugName.empty()) {
        params_.debugName = "BulletCompatibility";
    }
    accumulator_ = 0.0;
    lastSubStepCount_ = 0;
}

void BulletPhysicsEngine::StepSimulation(UnifiedSystem& system, EntityManager& entityManager, double dt) {
    unifiedSystem_ = &system;
    accumulator_ += dt;
    const double step = params_.fixedTimeStep > 0.0 ? params_.fixedTimeStep : dt;
    const unsigned int maxSteps = std::max(1u, params_.maxSubSteps);

    unsigned int performed = 0;
    while (accumulator_ + 1e-9 >= step && performed < maxSteps) {
        // Implement Bullet-style physics simulation
        // For now, delegate to built-in physics with enhanced features
        RunBulletSimulation(entityManager, step);
        accumulator_ -= step;
        ++performed;
    }

    if (performed == 0) {
        RunBulletSimulation(entityManager, dt);
        accumulator_ = 0.0;
        performed = 1;
    } else if (accumulator_ > 1e-6) {
        RunBulletSimulation(entityManager, accumulator_);
        accumulator_ = 0.0;
        ++performed;
    }

    lastSubStepCount_ = performed;
}

void BulletPhysicsEngine::RunBulletSimulation(EntityManager& entityManager, double dt) {
    // Enhanced physics simulation with Bullet-style features
    // This implements physics simulation directly, similar to what UnifiedSystem does
    // but with potential enhancements for Bullet-specific features
    
    // Apply gravity to all entities with RigidBody components
    entityManager.ForEach<Position, Velocity, RigidBody>([&](Entity /*entity*/, Position& /*pos*/, Velocity& vel, RigidBody& body) {
        if (body.inverseMass > 0.0f) { // Only apply gravity to non-static objects
            vel.vx += unifiedSystem_->GetGravityX() * dt;
            vel.vy += unifiedSystem_->GetGravityY() * dt; 
            vel.vz += unifiedSystem_->GetGravityZ() * dt;
        }
    });
    
    // Apply forces and integrate velocities
    entityManager.ForEach<Position, Velocity>([&](Entity entity, Position& pos, Velocity& vel) {
        // Integrate velocity to position
        pos.x += vel.vx * dt;
        pos.y += vel.vy * dt;
        pos.z += vel.vz * dt;
        
        // Apply damping (using default values since getters don't exist)
        const double linearDamping = 0.01;
        const double angularDamping = 0.01;
        vel.vx *= (1.0 - linearDamping * dt);
        vel.vy *= (1.0 - linearDamping * dt);
        vel.vz *= (1.0 - linearDamping * dt);
        
        // Apply angular damping to RigidBody if present
        if (auto* body = entityManager.GetComponent<RigidBody>(entity)) {
            body->angularVelocityX *= (1.0 - angularDamping * dt);
            body->angularVelocityY *= (1.0 - angularDamping * dt);
            body->angularVelocityZ *= (1.0 - angularDamping * dt);
            
            // Update orientation from angular velocity
            body->rotationX += body->angularVelocityX * dt;
            body->rotationY += body->angularVelocityY * dt;
            body->rotationZ += body->angularVelocityZ * dt;
        }
    });
    
    // Basic collision detection (simplified - assuming collision is enabled)
    // This would be enhanced with Bullet's collision detection in a full implementation
    entityManager.ForEach<Position, BoxCollider>([&](Entity /*entity*/, Position& pos, BoxCollider& /*collider*/) {
        // Simple boundary checking as a placeholder
        if (pos.x < -100.0) { pos.x = -100.0; }
        if (pos.x > 100.0) { pos.x = 100.0; }
        if (pos.y < -100.0) { pos.y = -100.0; }
        if (pos.y > 100.0) { pos.y = 100.0; }
        if (pos.z < -100.0) { pos.z = -100.0; }
        if (pos.z > 100.0) { pos.z = 100.0; }
    });
}

std::optional<RaycastHit> BulletPhysicsEngine::Raycast(double originX, double originY, double originZ,
                                                       double dirX, double dirY, double dirZ,
                                                       double maxDistance) {
    if (!unifiedSystem_) {
        return std::nullopt;
    }
    
    // For now, delegate to UnifiedSystem's raycast
    // In a full Bullet implementation, this would use Bullet's raycasting
    physics::RaycastHit hit;
    if (unifiedSystem_->Raycast(originX, originY, originZ, dirX, dirY, dirZ, maxDistance, hit)) {
        return hit;
    }
    return std::nullopt;
}

} // namespace physics
