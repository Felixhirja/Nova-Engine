#include "PhysicsSystem.h"
#include "Components.h"
#include <algorithm>
#include <cmath>
#include <iostream>

PhysicsSystem::PhysicsSystem(EntityManager* em)
    : entityManager_(em)
{
}

void PhysicsSystem::UseExternalEngine(std::shared_ptr<physics::IPhysicsEngine> engine) {
    externalEngine_ = std::move(engine);
    if (externalEngine_) {
        activeBackend_ = externalEngine_->GetBackendType();
    } else {
        activeBackend_ = physics::PhysicsBackendType::BuiltIn;
    }
}

void PhysicsSystem::ResetToBuiltin() {
    externalEngine_.reset();
    activeBackend_ = physics::PhysicsBackendType::BuiltIn;
}

void PhysicsSystem::StepWithBuiltin(EntityManager& entityManager, double dt) {
    entityManager_ = &entityManager;
    RunBuiltinSimulation(dt);
}

void PhysicsSystem::Update(EntityManager& entityManager, double dt) {
    // Update the stored pointer to match the passed reference
    entityManager_ = &entityManager;

    if (externalEngine_) {
        externalEngine_->StepSimulation(*this, entityManager, dt);
    } else {
        RunBuiltinSimulation(dt);
    }
}

void PhysicsSystem::RunBuiltinSimulation(double dt) {
    // Physics pipeline
    ApplyGravity(dt);
    ApplyConstantForces(dt);
    ApplyForces(dt);
    IntegrateVelocities(dt);

    if (collisionEnabled_) {
        DetectCollisions();
        ResolveCollisions(dt);
    }

    UpdateCharacterControllers(dt);
    UpdateJoints(dt);
    ClearFrameForces();
}

void PhysicsSystem::SetGravity(double x, double y, double z) {
    globalGravityX_ = x;
    globalGravityY_ = y;
    globalGravityZ_ = z;
}

void PhysicsSystem::SetGlobalDamping(double linear, double angular) {
    globalLinearDamping_ = linear;
    globalAngularDamping_ = angular;
}

void PhysicsSystem::SetMaxVelocity(double maxVel) {
    maxVelocity_ = maxVel;
}

void PhysicsSystem::SetCollisionEnabled(bool enabled) {
    collisionEnabled_ = enabled;
}

void PhysicsSystem::ApplyGravity(double dt) {
    // Apply global gravity to all RigidBodies with useGravity enabled
    entityManager_->ForEach<RigidBody, Position, Velocity>(
        [this, dt](Entity e, RigidBody& rb, Position& pos, Velocity& vel) {
            if (rb.useGravity && !rb.isKinematic) {
                vel.vx += globalGravityX_ * dt;
                vel.vy += globalGravityY_ * dt;
                vel.vz += globalGravityZ_ * dt;
            }
        });
    
    // Apply gravity from GravitySource components
    auto gravitySources = entityManager_->GetAllWith<GravitySource>();
    for (const auto& [sourceEntity, source] : gravitySources) {
        auto* sourcePos = entityManager_->GetComponent<Position>(sourceEntity);
        if (!sourcePos) continue;
        
        entityManager_->ForEach<RigidBody, Position, Velocity>(
            [this, dt, source, sourcePos](Entity e, RigidBody& rb, Position& pos, Velocity& vel) {
                if (!rb.useGravity || rb.isKinematic) return;
                
                if (source->isUniform) {
                    // Uniform gravity (like Earth's surface)
                    vel.vx += source->directionX * source->strength * dt;
                    vel.vy += source->directionY * source->strength * dt;
                    vel.vz += source->directionZ * source->strength * dt;
                } else {
                    // Point gravity (like a planet/star)
                    double dx = sourcePos->x - pos.x;
                    double dy = sourcePos->y - pos.y;
                    double dz = sourcePos->z - pos.z;
                    double distSq = dx*dx + dy*dy + dz*dz;
                    
                    if (distSq < 0.0001) return;  // Too close, skip
                    if (source->radius > 0.0 && distSq > source->radius * source->radius) return;  // Too far
                    
                    double dist = std::sqrt(distSq);
                    double force = source->strength / distSq;  // Inverse square law
                    
                    double nx = dx / dist;
                    double ny = dy / dist;
                    double nz = dz / dist;
                    
                    vel.vx += nx * force * dt;
                    vel.vy += ny * force * dt;
                    vel.vz += nz * force * dt;
                }
            });
    }
}

void PhysicsSystem::ApplyConstantForces(double dt) {
    entityManager_->ForEach<RigidBody, ConstantForce, Velocity>(
        [dt](Entity e, RigidBody& rb, ConstantForce& cf, Velocity& vel) {
            if (rb.isKinematic) return;
            
            double fx = cf.forceX;
            double fy = cf.forceY;
            double fz = cf.forceZ;
            
            // TODO: Transform force if in local space
            
            // F = ma, so a = F/m
            double ax = fx * rb.inverseMass;
            double ay = fy * rb.inverseMass;
            double az = fz * rb.inverseMass;
            
            vel.vx += ax * dt;
            vel.vy += ay * dt;
            vel.vz += az * dt;
        });
}

void PhysicsSystem::ApplyForces(double dt) {
    entityManager_->ForEach<RigidBody, Force, Velocity>(
        [dt](Entity e, RigidBody& rb, Force& force, Velocity& vel) {
            if (rb.isKinematic) return;
            
            switch (force.mode) {
                case Force::Mode::Force: {
                    // F = ma, so a = F/m
                    double ax = force.fx * rb.inverseMass;
                    double ay = force.fy * rb.inverseMass;
                    double az = force.fz * rb.inverseMass;
                    vel.vx += ax * dt;
                    vel.vy += ay * dt;
                    vel.vz += az * dt;
                    break;
                }
                case Force::Mode::Impulse: {
                    // Instantaneous velocity change: dv = impulse / mass
                    vel.vx += force.fx * rb.inverseMass;
                    vel.vy += force.fy * rb.inverseMass;
                    vel.vz += force.fz * rb.inverseMass;
                    force.lifetime = 0.0;  // Mark for removal
                    break;
                }
                case Force::Mode::Acceleration: {
                    // Direct acceleration (ignores mass)
                    vel.vx += force.fx * dt;
                    vel.vy += force.fy * dt;
                    vel.vz += force.fz * dt;
                    break;
                }
                case Force::Mode::VelocityChange: {
                    // Direct velocity change (ignores mass)
                    vel.vx += force.fx;
                    vel.vy += force.fy;
                    vel.vz += force.fz;
                    force.lifetime = 0.0;  // Mark for removal
                    break;
                }
            }
            
            // Update lifetime
            if (force.lifetime > 0.0) {
                force.lifetime -= dt;
                if (force.lifetime <= 0.0) {
                    force.lifetime = 0.0;
                }
            }
        });
}

void PhysicsSystem::IntegrateVelocities(double dt) {
    entityManager_->ForEach<RigidBody, Position, Velocity>(
        [this, dt](Entity e, RigidBody& rb, Position& pos, Velocity& vel) {
            if (rb.isKinematic) return;
            
            // Apply damping
            double linearDamp = rb.linearDamping > 0.0 ? rb.linearDamping : globalLinearDamping_;
            double dampFactor = 1.0 / (1.0 + linearDamp * dt);
            vel.vx *= dampFactor;
            vel.vy *= dampFactor;
            vel.vz *= dampFactor;
            
            // Clamp velocity to maximum
            double speedSq = vel.vx*vel.vx + vel.vy*vel.vy + vel.vz*vel.vz;
            if (speedSq > maxVelocity_ * maxVelocity_) {
                double speed = std::sqrt(speedSq);
                double scale = maxVelocity_ / speed;
                vel.vx *= scale;
                vel.vy *= scale;
                vel.vz *= scale;
            }
            
            // Integrate position
            if (!rb.freezePositionX) pos.x += vel.vx * dt;
            if (!rb.freezePositionY) pos.y += vel.vy * dt;
            if (!rb.freezePositionZ) pos.z += vel.vz * dt;
            
            // Integrate rotation (simplified Euler integration)
            double angularDamp = rb.angularDamping > 0.0 ? rb.angularDamping : globalAngularDamping_;
            double angDampFactor = 1.0 / (1.0 + angularDamp * dt);
            rb.angularVelocityX *= angDampFactor;
            rb.angularVelocityY *= angDampFactor;
            rb.angularVelocityZ *= angDampFactor;
            
            if (!rb.freezeRotationX) rb.rotationX += rb.angularVelocityX * dt;
            if (!rb.freezeRotationY) rb.rotationY += rb.angularVelocityY * dt;
            if (!rb.freezeRotationZ) rb.rotationZ += rb.angularVelocityZ * dt;
        });
}

void PhysicsSystem::DetectCollisions() {
    // Clear previous frame collisions
    entityManager_->ForEach<CollisionInfo>([](Entity e, CollisionInfo& info) {
        info.Clear();
    });
    
    // Detect new collisions
    auto collisionPairs = DetectCollisionPairs();
    
    // Store collision information
    for (const auto& pair : collisionPairs) {
        // Add to entity A's collision info
        auto* infoA = entityManager_->GetComponent<CollisionInfo>(pair.entityA);
        if (!infoA) {
            entityManager_->EmplaceComponent<CollisionInfo>(pair.entityA);
            infoA = entityManager_->GetComponent<CollisionInfo>(pair.entityA);
        }
        
        if (infoA) {
            CollisionInfo::Contact contact;
            contact.otherEntity = pair.entityB;
            contact.normalX = pair.normalX;
            contact.normalY = pair.normalY;
            contact.normalZ = pair.normalZ;
            contact.penetrationDepth = pair.penetration;
            contact.contactPointX = pair.contactX;
            contact.contactPointY = pair.contactY;
            contact.contactPointZ = pair.contactZ;
            infoA->contacts.push_back(contact);
            infoA->collisionCount++;
        }
        
        // Add to entity B's collision info (with inverted normal)
        auto* infoB = entityManager_->GetComponent<CollisionInfo>(pair.entityB);
        if (!infoB) {
            entityManager_->EmplaceComponent<CollisionInfo>(pair.entityB);
            infoB = entityManager_->GetComponent<CollisionInfo>(pair.entityB);
        }
        
        if (infoB) {
            CollisionInfo::Contact contact;
            contact.otherEntity = pair.entityA;
            contact.normalX = -pair.normalX;
            contact.normalY = -pair.normalY;
            contact.normalZ = -pair.normalZ;
            contact.penetrationDepth = pair.penetration;
            contact.contactPointX = pair.contactX;
            contact.contactPointY = pair.contactY;
            contact.contactPointZ = pair.contactZ;
            infoB->contacts.push_back(contact);
            infoB->collisionCount++;
        }
    }
}

std::vector<PhysicsSystem::CollisionPair> PhysicsSystem::DetectCollisionPairs() {
    std::vector<CollisionPair> pairs;
    
    // Get all entities with colliders and positions
    auto boxColliders = entityManager_->GetAllWith<BoxCollider>();
    auto sphereColliders = entityManager_->GetAllWith<SphereCollider>();
    
    // Box vs Box
    for (size_t i = 0; i < boxColliders.size(); ++i) {
        for (size_t j = i + 1; j < boxColliders.size(); ++j) {
            auto [entityA, colliderA] = boxColliders[i];
            auto [entityB, colliderB] = boxColliders[j];
            
            if (!colliderA->isEnabled || !colliderB->isEnabled) continue;
            
            // Check layer mask
            if (!(colliderA->collisionLayer & colliderB->collisionMask)) continue;
            if (!(colliderB->collisionLayer & colliderA->collisionMask)) continue;
            
            auto* posA = entityManager_->GetComponent<Position>(entityA);
            auto* posB = entityManager_->GetComponent<Position>(entityB);
            if (!posA || !posB) continue;
            
            CollisionPair pair;
            if (CheckBoxBox(*colliderA, *posA, *colliderB, *posB, pair)) {
                pair.entityA = entityA;
                pair.entityB = entityB;
                pairs.push_back(pair);
            }
        }
    }
    
    // Sphere vs Sphere
    for (size_t i = 0; i < sphereColliders.size(); ++i) {
        for (size_t j = i + 1; j < sphereColliders.size(); ++j) {
            auto [entityA, colliderA] = sphereColliders[i];
            auto [entityB, colliderB] = sphereColliders[j];
            
            if (!colliderA->isEnabled || !colliderB->isEnabled) continue;
            
            if (!(colliderA->collisionLayer & colliderB->collisionMask)) continue;
            if (!(colliderB->collisionLayer & colliderA->collisionMask)) continue;
            
            auto* posA = entityManager_->GetComponent<Position>(entityA);
            auto* posB = entityManager_->GetComponent<Position>(entityB);
            if (!posA || !posB) continue;
            
            CollisionPair pair;
            if (CheckSphereSphere(*colliderA, *posA, *colliderB, *posB, pair)) {
                pair.entityA = entityA;
                pair.entityB = entityB;
                pairs.push_back(pair);
            }
        }
    }
    
    // Box vs Sphere
    for (const auto& [boxEntity, boxCollider] : boxColliders) {
        for (const auto& [sphereEntity, sphereCollider] : sphereColliders) {
            if (!boxCollider->isEnabled || !sphereCollider->isEnabled) continue;
            
            if (!(boxCollider->collisionLayer & sphereCollider->collisionMask)) continue;
            if (!(sphereCollider->collisionLayer & boxCollider->collisionMask)) continue;
            
            auto* boxPos = entityManager_->GetComponent<Position>(boxEntity);
            auto* spherePos = entityManager_->GetComponent<Position>(sphereEntity);
            if (!boxPos || !spherePos) continue;
            
            CollisionPair pair;
            if (CheckBoxSphere(*boxCollider, *boxPos, *sphereCollider, *spherePos, pair)) {
                pair.entityA = boxEntity;
                pair.entityB = sphereEntity;
                pairs.push_back(pair);
            }
        }
    }
    
    return pairs;
}

bool PhysicsSystem::CheckBoxBox(const BoxCollider& a, const Position& posA,
                                  const BoxCollider& b, const Position& posB,
                                  CollisionPair& result) {
    // Simple AABB collision detection
    double aMinX = posA.x + a.offsetX - a.width * 0.5;
    double aMaxX = posA.x + a.offsetX + a.width * 0.5;
    double aMinY = posA.y + a.offsetY - a.height * 0.5;
    double aMaxY = posA.y + a.offsetY + a.height * 0.5;
    double aMinZ = posA.z + a.offsetZ - a.depth * 0.5;
    double aMaxZ = posA.z + a.offsetZ + a.depth * 0.5;
    
    double bMinX = posB.x + b.offsetX - b.width * 0.5;
    double bMaxX = posB.x + b.offsetX + b.width * 0.5;
    double bMinY = posB.y + b.offsetY - b.height * 0.5;
    double bMaxY = posB.y + b.offsetY + b.height * 0.5;
    double bMinZ = posB.z + b.offsetZ - b.depth * 0.5;
    double bMaxZ = posB.z + b.offsetZ + b.depth * 0.5;
    
    // Check for overlap on all axes
    bool overlapX = aMaxX >= bMinX && bMaxX >= aMinX;
    bool overlapY = aMaxY >= bMinY && bMaxY >= aMinY;
    bool overlapZ = aMaxZ >= bMinZ && bMaxZ >= aMinZ;
    
    if (!overlapX || !overlapY || !overlapZ) {
        return false;  // No collision
    }
    
    // Calculate penetration depth on each axis
    double penetX = std::min(aMaxX - bMinX, bMaxX - aMinX);
    double penetY = std::min(aMaxY - bMinY, bMaxY - aMinY);
    double penetZ = std::min(aMaxZ - bMinZ, bMaxZ - aMinZ);
    
    // Find axis of minimum penetration (collision normal)
    if (penetX < penetY && penetX < penetZ) {
        result.normalX = (posA.x < posB.x) ? 1.0 : -1.0;
        result.normalY = 0.0;
        result.normalZ = 0.0;
        result.penetration = penetX;
    } else if (penetY < penetZ) {
        result.normalX = 0.0;
        result.normalY = (posA.y < posB.y) ? 1.0 : -1.0;
        result.normalZ = 0.0;
        result.penetration = penetY;
    } else {
        result.normalX = 0.0;
        result.normalY = 0.0;
        result.normalZ = (posA.z < posB.z) ? 1.0 : -1.0;
        result.penetration = penetZ;
    }
    
    // Contact point (midpoint between boxes)
    result.contactX = (posA.x + posB.x) * 0.5;
    result.contactY = (posA.y + posB.y) * 0.5;
    result.contactZ = (posA.z + posB.z) * 0.5;
    
    return true;
}

bool PhysicsSystem::CheckSphereSphere(const SphereCollider& a, const Position& posA,
                                       const SphereCollider& b, const Position& posB,
                                       CollisionPair& result) {
    double dx = (posB.x + b.offsetX) - (posA.x + a.offsetX);
    double dy = (posB.y + b.offsetY) - (posA.y + a.offsetY);
    double dz = (posB.z + b.offsetZ) - (posA.z + a.offsetZ);
    
    double distSq = dx*dx + dy*dy + dz*dz;
    double radiusSum = a.radius + b.radius;
    
    if (distSq >= radiusSum * radiusSum) {
        return false;  // No collision
    }
    
    double dist = std::sqrt(distSq);
    
    // Collision normal (from A to B)
    if (dist > 0.0001) {
        result.normalX = dx / dist;
        result.normalY = dy / dist;
        result.normalZ = dz / dist;
    } else {
        // Spheres at same position, use arbitrary normal
        result.normalX = 1.0;
        result.normalY = 0.0;
        result.normalZ = 0.0;
    }
    
    result.penetration = radiusSum - dist;
    
    // Contact point (on surface of sphere A)
    result.contactX = posA.x + a.offsetX + result.normalX * a.radius;
    result.contactY = posA.y + a.offsetY + result.normalY * a.radius;
    result.contactZ = posA.z + a.offsetZ + result.normalZ * a.radius;
    
    return true;
}

bool PhysicsSystem::CheckBoxSphere(const BoxCollider& box, const Position& boxPos,
                                     const SphereCollider& sphere, const Position& spherePos,
                                     CollisionPair& result) {
    // Find closest point on box to sphere center
    double sphereCenterX = spherePos.x + sphere.offsetX;
    double sphereCenterY = spherePos.y + sphere.offsetY;
    double sphereCenterZ = spherePos.z + sphere.offsetZ;
    
    double boxMinX = boxPos.x + box.offsetX - box.width * 0.5;
    double boxMaxX = boxPos.x + box.offsetX + box.width * 0.5;
    double boxMinY = boxPos.y + box.offsetY - box.height * 0.5;
    double boxMaxY = boxPos.y + box.offsetY + box.height * 0.5;
    double boxMinZ = boxPos.z + box.offsetZ - box.depth * 0.5;
    double boxMaxZ = boxPos.z + box.offsetZ + box.depth * 0.5;
    
    double closestX = Clamp(sphereCenterX, boxMinX, boxMaxX);
    double closestY = Clamp(sphereCenterY, boxMinY, boxMaxY);
    double closestZ = Clamp(sphereCenterZ, boxMinZ, boxMaxZ);
    
    // Vector from closest point to sphere center
    double dx = sphereCenterX - closestX;
    double dy = sphereCenterY - closestY;
    double dz = sphereCenterZ - closestZ;
    
    double distSq = dx*dx + dy*dy + dz*dz;
    
    if (distSq >= sphere.radius * sphere.radius) {
        return false;  // No collision
    }
    
    double dist = std::sqrt(distSq);
    
    if (dist > 0.0001) {
        result.normalX = dx / dist;
        result.normalY = dy / dist;
        result.normalZ = dz / dist;
    } else {
        // Sphere center inside box, use direction to box center
        double toCenterX = (boxPos.x + box.offsetX) - sphereCenterX;
        double toCenterY = (boxPos.y + box.offsetY) - sphereCenterY;
        double toCenterZ = (boxPos.z + box.offsetZ) - sphereCenterZ;
        double len = VectorLength(toCenterX, toCenterY, toCenterZ);
        if (len > 0.0001) {
            result.normalX = -toCenterX / len;
            result.normalY = -toCenterY / len;
            result.normalZ = -toCenterZ / len;
        } else {
            result.normalX = 0.0;
            result.normalY = 0.0;
            result.normalZ = 1.0;
        }
    }
    
    result.penetration = sphere.radius - dist;
    result.contactX = closestX;
    result.contactY = closestY;
    result.contactZ = closestZ;
    
    return true;
}

void PhysicsSystem::ResolveCollisions(double dt) {
    auto collisionPairs = DetectCollisionPairs();
    
    for (const auto& pair : collisionPairs) {
        auto* colliderA = entityManager_->GetComponent<Collider>(pair.entityA);
        auto* colliderB = entityManager_->GetComponent<Collider>(pair.entityB);
        
        // Skip trigger colliders
        if ((colliderA && colliderA->isTrigger) || (colliderB && colliderB->isTrigger)) {
            continue;
        }
        
        ResolveCollisionPair(pair, dt);
    }
}

void PhysicsSystem::ResolveCollisionPair(const CollisionPair& pair, double dt) {
    auto* rbA = entityManager_->GetComponent<RigidBody>(pair.entityA);
    auto* rbB = entityManager_->GetComponent<RigidBody>(pair.entityB);
    auto* velA = entityManager_->GetComponent<Velocity>(pair.entityA);
    auto* velB = entityManager_->GetComponent<Velocity>(pair.entityB);
    
    // Separate objects
    SeparateColliders(pair.entityA, pair.entityB,
                     pair.normalX, pair.normalY, pair.normalZ,
                     pair.penetration);
    
    if (!velA || !velB) return;
    if ((rbA && rbA->isKinematic) && (rbB && rbB->isKinematic)) return;
    
    // Calculate relative velocity
    double relVelX = velB->vx - velA->vx;
    double relVelY = velB->vy - velA->vy;
    double relVelZ = velB->vz - velA->vz;
    
    // Velocity along collision normal
    double velAlongNormal = DotProduct(relVelX, relVelY, relVelZ,
                                      pair.normalX, pair.normalY, pair.normalZ);
    
    // Objects moving apart, don't resolve
    if (velAlongNormal > 0.0) return;
    
    // Calculate restitution (bounciness)
    double restitution = 0.5;
    if (rbA && rbB) {
        restitution = std::min(rbA->restitution, rbB->restitution);
    } else if (rbA) {
        restitution = rbA->restitution;
    } else if (rbB) {
        restitution = rbB->restitution;
    }
    
    // Calculate impulse scalar
    double j = -(1.0 + restitution) * velAlongNormal;
    
    double invMassA = (rbA && !rbA->isKinematic) ? rbA->inverseMass : 0.0;
    double invMassB = (rbB && !rbB->isKinematic) ? rbB->inverseMass : 0.0;
    double invMassSum = invMassA + invMassB;
    
    if (invMassSum > 0.0) {
        j /= invMassSum;

        // Apply impulse
        if (rbA && !rbA->isKinematic) {
            velA->vx -= j * invMassA * pair.normalX;
            velA->vy -= j * invMassA * pair.normalY;
            velA->vz -= j * invMassA * pair.normalZ;
        }

        if (rbB && !rbB->isKinematic) {
            velB->vx += j * invMassB * pair.normalX;
            velB->vy += j * invMassB * pair.normalY;
            velB->vz += j * invMassB * pair.normalZ;
        }
    }

    auto updateGrounding = [&](unsigned int entity, double verticalComponent) {
        auto* playerPhysics = entityManager_->GetComponent<PlayerPhysics>(entity);
        if (!playerPhysics) return;

        if (verticalComponent > 0.5) {
            playerPhysics->isGrounded = true;
            if (auto* entityVelocity = entityManager_->GetComponent<Velocity>(entity)) {
                if (entityVelocity->vz < 0.0) {
                    entityVelocity->vz = 0.0;
                }
            }
        }
    };

    updateGrounding(pair.entityA, -pair.normalZ);
    updateGrounding(pair.entityB, pair.normalZ);
}

void PhysicsSystem::SeparateColliders(unsigned int entityA, unsigned int entityB,
                                       double normalX, double normalY, double normalZ,
                                       double penetration) {
    auto* rbA = entityManager_->GetComponent<RigidBody>(entityA);
    auto* rbB = entityManager_->GetComponent<RigidBody>(entityB);
    auto* posA = entityManager_->GetComponent<Position>(entityA);
    auto* posB = entityManager_->GetComponent<Position>(entityB);
    
    if (!posA || !posB) return;
    
    double invMassA = (rbA && !rbA->isKinematic) ? rbA->inverseMass : 0.0;
    double invMassB = (rbB && !rbB->isKinematic) ? rbB->inverseMass : 0.0;
    double invMassSum = invMassA + invMassB;
    
    if (invMassSum <= 0.0) return;  // Both kinematic
    
    // Separate proportional to inverse mass
    double separationA = penetration * (invMassA / invMassSum);
    double separationB = penetration * (invMassB / invMassSum);
    
    if (rbA && !rbA->isKinematic) {
        posA->x -= normalX * separationA;
        posA->y -= normalY * separationA;
        posA->z -= normalZ * separationA;
    }
    
    if (rbB && !rbB->isKinematic) {
        posB->x += normalX * separationB;
        posB->y += normalY * separationB;
        posB->z += normalZ * separationB;
    }
}

void PhysicsSystem::UpdateCharacterControllers(double dt) {
    // TODO: Implement character controller physics
    // This would handle ground detection, step climbing, slope limits, etc.
}

void PhysicsSystem::UpdateJoints(double dt) {
    // TODO: Implement joint constraints
    // This would maintain distance/angle constraints between connected bodies
}

void PhysicsSystem::ClearFrameForces() {
    // Remove one-shot forces that have expired
    auto forces = entityManager_->GetAllWith<Force>();
    for (auto [entity, force] : forces) {
        if (force->lifetime == 0.0) {
            entityManager_->RemoveComponent<Force>(entity);
        }
    }
}

// Helper methods
double PhysicsSystem::DotProduct(double ax, double ay, double az, 
                                 double bx, double by, double bz) const {
    return ax*bx + ay*by + az*bz;
}

double PhysicsSystem::VectorLength(double x, double y, double z) const {
    return std::sqrt(x*x + y*y + z*z);
}

void PhysicsSystem::Normalize(double& x, double& y, double& z) const {
    double len = VectorLength(x, y, z);
    if (len > 0.0001) {
        x /= len;
        y /= len;
        z /= len;
    }
}

double PhysicsSystem::Clamp(double value, double min, double max) const {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

// Force application methods
void PhysicsSystem::ApplyForce(unsigned int entity, double fx, double fy, double fz) {
    if (!entityManager_->IsAlive(entity)) return;
    
    auto* force = entityManager_->GetComponent<Force>(entity);
    if (!force) {
        force = &entityManager_->EmplaceComponent<Force>(entity);
    }
    
    force->fx += fx;
    force->fy += fy;
    force->fz += fz;
    force->mode = Force::Mode::Force;
}

void PhysicsSystem::ApplyImpulse(unsigned int entity, double ix, double iy, double iz) {
    if (!entityManager_->IsAlive(entity)) return;
    
    auto& force = entityManager_->EmplaceComponent<Force>(entity);
    force.fx = ix;
    force.fy = iy;
    force.fz = iz;
    force.mode = Force::Mode::Impulse;
    force.lifetime = 0.0;
}

void PhysicsSystem::ApplyForceAtPoint(unsigned int entity,double fx, double fy, double fz,
                                       double px, double py, double pz) {
    // For now, just apply the force (torque calculation would go here)
    ApplyForce(entity, fx, fy, fz);
    // TODO: Calculate and apply torque based on point offset from center of mass
}

// Collision queries
bool PhysicsSystem::Raycast(double originX, double originY, double originZ,
                            double dirX, double dirY, double dirZ,
                            double maxDistance, RaycastHit& hit) {
    // TODO: Implement raycasting
    // This would test the ray against all colliders and find the closest hit
    return false;
}

std::vector<unsigned int> PhysicsSystem::OverlapSphere(double centerX, double centerY, double centerZ,
                                                       double radius, unsigned int layerMask) {
    std::vector<unsigned int> results;
    
    // Check all sphere colliders
    auto sphereColliders = entityManager_->GetAllWith<SphereCollider>();
    for (const auto& [entity, collider] : sphereColliders) {
        if (!collider->isEnabled) continue;
        if (!(collider->collisionLayer & layerMask)) continue;
        
        auto* pos = entityManager_->GetComponent<Position>(entity);
        if (!pos) continue;
        
        double dx = (pos->x + collider->offsetX) - centerX;
        double dy = (pos->y + collider->offsetY) - centerY;
        double dz = (pos->z + collider->offsetZ) - centerZ;
        double distSq = dx*dx + dy*dy + dz*dz;
        double radiusSum = radius + collider->radius;
        
        if (distSq < radiusSum * radiusSum) {
            results.push_back(entity);
        }
    }
    
    // TODO: Check box colliders and other shapes
    
    return results;
}

std::vector<unsigned int> PhysicsSystem::OverlapBox(double centerX, double centerY, double centerZ,
                                                     double width, double height, double depth,
                                                     unsigned int layerMask) {
    std::vector<unsigned int> results;
    
    // TODO: Implement box overlap query
    
    return results;
}
