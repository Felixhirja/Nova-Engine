#pragma once
#include "System.h"
#include "EntityManager.h"
#include <vector>
#include <cmath>
#include <memory>

#include "../physics/PhysicsEngine.h"

/**
 * PhysicsSystem: Handles physics simulation for entities with RigidBody components
 * 
 * Responsibilities:
 * - Apply forces and update velocities
 * - Integrate velocities to update positions
 * - Apply gravity from GravitySource components
 * - Handle collision detection and response
 * - Manage constraints (joints, character controllers)
 */
class PhysicsSystem : public System {
public:
    PhysicsSystem(EntityManager* em);

    void Update(EntityManager& entityManager, double dt) override;

    void UseExternalEngine(std::shared_ptr<physics::IPhysicsEngine> engine);
    void ResetToBuiltin();
    physics::PhysicsBackendType GetActiveBackendType() const noexcept { return activeBackend_; }
    std::shared_ptr<physics::IPhysicsEngine> GetActiveEngine() const noexcept { return externalEngine_; }
    void StepWithBuiltin(EntityManager& entityManager, double dt);
    
    // Configuration
    void SetGravity(double x, double y, double z);
    void SetGlobalDamping(double linear, double angular);
    void SetMaxVelocity(double maxVel);
    void SetCollisionEnabled(bool enabled);
    
    // Collision queries
    struct RaycastHit {
        unsigned int entity = 0;
        double distance = 0.0;
        double hitPointX = 0.0;
        double hitPointY = 0.0;
        double hitPointZ = 0.0;
        double normalX = 0.0;
        double normalY = 0.0;
        double normalZ = 0.0;
    };
    
    bool Raycast(double originX, double originY, double originZ,
                 double dirX, double dirY, double dirZ,
                 double maxDistance, RaycastHit& hit);
    
    std::vector<unsigned int> OverlapSphere(double centerX, double centerY, double centerZ,
                                            double radius, unsigned int layerMask = 0xFFFFFFFF);
    
    std::vector<unsigned int> OverlapBox(double centerX, double centerY, double centerZ,
                                         double width, double height, double depth,
                                         unsigned int layerMask = 0xFFFFFFFF);
    
    // Force application helpers
    void ApplyForce(unsigned int entity, double fx, double fy, double fz);
    void ApplyImpulse(unsigned int entity, double ix, double iy, double iz);
    void ApplyForceAtPoint(unsigned int entity, double fx, double fy, double fz,
                          double px, double py, double pz);
    
    // Getters
    double GetGravityX() const { return globalGravityX_; }
    double GetGravityY() const { return globalGravityY_; }
    double GetGravityZ() const { return globalGravityZ_; }
    
private:
    EntityManager* entityManager_;
    std::shared_ptr<physics::IPhysicsEngine> externalEngine_;
    physics::PhysicsBackendType activeBackend_ = physics::PhysicsBackendType::BuiltIn;
    
    // Global physics settings
    double globalGravityX_ = 0.0;
    double globalGravityY_ = 0.0;
    double globalGravityZ_ = -9.8;
    double globalLinearDamping_ = 0.01;
    double globalAngularDamping_ = 0.01;
    double maxVelocity_ = 100.0;
    bool collisionEnabled_ = true;
    
    // Update phases
    void ApplyGravity(double dt);
    void ApplyForces(double dt);
    void ApplyConstantForces(double dt);
    void IntegrateVelocities(double dt);
    void DetectCollisions(double dt);
    void ResolveCollisions(double dt);
    void UpdateCharacterControllers(double dt);
    void UpdateJoints(double dt);
    void ClearFrameForces();

    void RunBuiltinSimulation(double dt);
    
    // Collision detection helpers
    struct CollisionPair {
        unsigned int entityA;
        unsigned int entityB;
        double normalX, normalY, normalZ;
        double penetration;
        double contactX, contactY, contactZ;
        double timeOfImpact = 0.0;
        bool dynamic = false;
    };

    std::vector<CollisionPair> DetectCollisionPairs();
    std::vector<CollisionPair> DetectSweptCollisionPairs(double dt);
    bool ComputeSweptAABB(const struct BoxCollider& a, const struct Position& posA,
                          const struct Velocity* velA,
                          const struct BoxCollider& b, const struct Position& posB,
                          const struct Velocity* velB, double dt,
                          CollisionPair& result);
    bool CheckBoxBox(const struct BoxCollider& a, const struct Position& posA,
                     const struct BoxCollider& b, const struct Position& posB,
                     CollisionPair& result);
    bool CheckSphereSphere(const struct SphereCollider& a, const struct Position& posA,
                          const struct SphereCollider& b, const struct Position& posB,
                          CollisionPair& result);
    bool CheckBoxSphere(const struct BoxCollider& box, const struct Position& boxPos,
                       const struct SphereCollider& sphere, const struct Position& spherePos,
                       CollisionPair& result);
    
    // Collision response
    void ResolveCollisionPair(const CollisionPair& pair, double dt);
    void SeparateColliders(unsigned int entityA, unsigned int entityB,
                          double normalX, double normalY, double normalZ,
                          double penetration);
    
    // Utility functions
    double DotProduct(double ax, double ay, double az, double bx, double by, double bz) const;
    double VectorLength(double x, double y, double z) const;
    void Normalize(double& x, double& y, double& z) const;
    double Clamp(double value, double min, double max) const;

    std::vector<CollisionPair> currentCollisions_;
};
