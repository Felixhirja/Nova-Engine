#include "PhysicsSystem.h"
#include "Components.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>

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
        DetectCollisions(dt);
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
                if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                    double mass = (rb.inverseMass > 0.0) ? (1.0 / rb.inverseMass) : rb.mass;
                    accum->accumulatedForceX += globalGravityX_ * mass;
                    accum->accumulatedForceY += globalGravityY_ * mass;
                    accum->accumulatedForceZ += globalGravityZ_ * mass;
                }
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
                    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                        double mass = (rb.inverseMass > 0.0) ? (1.0 / rb.inverseMass) : rb.mass;
                        accum->accumulatedForceX += source->directionX * source->strength * mass;
                        accum->accumulatedForceY += source->directionY * source->strength * mass;
                        accum->accumulatedForceZ += source->directionZ * source->strength * mass;
                    }
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
                    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                        double mass = (rb.inverseMass > 0.0) ? (1.0 / rb.inverseMass) : rb.mass;
                        accum->accumulatedForceX += nx * force * mass;
                        accum->accumulatedForceY += ny * force * mass;
                        accum->accumulatedForceZ += nz * force * mass;
                    }
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

            if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                accum->accumulatedForceX += fx;
                accum->accumulatedForceY += fy;
                accum->accumulatedForceZ += fz;
            }
        });
}

void PhysicsSystem::ApplyForces(double dt) {
    entityManager_->ForEach<RigidBody, Force, Velocity>(
        [this, dt](Entity e, RigidBody& rb, Force& force, Velocity& vel) {
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
                    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                        accum->accumulatedForceX += force.fx;
                        accum->accumulatedForceY += force.fy;
                        accum->accumulatedForceZ += force.fz;
                    }
                    break;
                }
                case Force::Mode::Impulse: {
                    // Instantaneous velocity change: dv = impulse / mass
                    vel.vx += force.fx * rb.inverseMass;
                    vel.vy += force.fy * rb.inverseMass;
                    vel.vz += force.fz * rb.inverseMass;
                    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                        accum->accumulatedImpulseX += force.fx * rb.inverseMass;
                        accum->accumulatedImpulseY += force.fy * rb.inverseMass;
                        accum->accumulatedImpulseZ += force.fz * rb.inverseMass;
                    }
                    force.lifetime = 0.0;  // Mark for removal
                    break;
                }
                case Force::Mode::Acceleration: {
                    // Direct acceleration (ignores mass)
                    vel.vx += force.fx * dt;
                    vel.vy += force.fy * dt;
                    vel.vz += force.fz * dt;
                    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                        double mass = (rb.inverseMass > 0.0) ? (1.0 / rb.inverseMass) : rb.mass;
                        accum->accumulatedForceX += force.fx * mass;
                        accum->accumulatedForceY += force.fy * mass;
                        accum->accumulatedForceZ += force.fz * mass;
                    }
                    break;
                }
                case Force::Mode::VelocityChange: {
                    // Direct velocity change (ignores mass)
                    vel.vx += force.fx;
                    vel.vy += force.fy;
                    vel.vz += force.fz;
                    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(e)) {
                        double mass = (rb.inverseMass > 0.0) ? (1.0 / rb.inverseMass) : rb.mass;
                        accum->accumulatedImpulseX += force.fx * mass;
                        accum->accumulatedImpulseY += force.fy * mass;
                        accum->accumulatedImpulseZ += force.fz * mass;
                    }
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

void PhysicsSystem::DetectCollisions(double dt) {
    // Clear previous frame collisions
    entityManager_->ForEach<CollisionInfo>([](Entity e, CollisionInfo& info) {
        info.Clear();
    });

    currentCollisions_.clear();

    auto staticPairs = DetectCollisionPairs();
    auto sweptPairs = DetectSweptCollisionPairs(dt);

    auto appendIfUnique = [this](const CollisionPair& candidate) {
        auto exists = std::find_if(currentCollisions_.begin(), currentCollisions_.end(),
                                   [&candidate](const CollisionPair& existing) {
                                       return (existing.entityA == candidate.entityA &&
                                               existing.entityB == candidate.entityB) ||
                                              (existing.entityA == candidate.entityB &&
                                               existing.entityB == candidate.entityA);
                                   });
        if (exists == currentCollisions_.end()) {
            currentCollisions_.push_back(candidate);
        }
    };

    for (auto& pair : staticPairs) {
        pair.timeOfImpact = 0.0;
        pair.dynamic = false;
        appendIfUnique(pair);
    }

    for (auto& pair : sweptPairs) {
        appendIfUnique(pair);
    }

    // Store collision information for systems that rely on contact data
    for (const auto& pair : currentCollisions_) {
        auto ensureInfo = [this](unsigned int entity) -> CollisionInfo* {
            auto* info = entityManager_->GetComponent<CollisionInfo>(entity);
            if (!info) {
                entityManager_->EmplaceComponent<CollisionInfo>(entity);
                info = entityManager_->GetComponent<CollisionInfo>(entity);
            }
            return info;
        };

        if (auto* infoA = ensureInfo(pair.entityA)) {
            CollisionInfo::Contact contact;
            contact.otherEntity = pair.entityB;
            contact.normalX = pair.normalX;
            contact.normalY = pair.normalY;
            contact.normalZ = pair.normalZ;
            contact.penetrationDepth = pair.penetration;
            contact.contactPointX = pair.contactX;
            contact.contactPointY = pair.contactY;
            contact.contactPointZ = pair.contactZ;
            contact.timeOfImpact = pair.timeOfImpact;
            contact.timestamp = pair.timeOfImpact * dt;
            infoA->contacts.push_back(contact);
            infoA->collisionCount++;
        }

        if (auto* infoB = ensureInfo(pair.entityB)) {
            CollisionInfo::Contact contact;
            contact.otherEntity = pair.entityA;
            contact.normalX = -pair.normalX;
            contact.normalY = -pair.normalY;
            contact.normalZ = -pair.normalZ;
            contact.penetrationDepth = pair.penetration;
            contact.contactPointX = pair.contactX;
            contact.contactPointY = pair.contactY;
            contact.contactPointZ = pair.contactZ;
            contact.timeOfImpact = pair.timeOfImpact;
            contact.timestamp = pair.timeOfImpact * dt;
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
                pair.timeOfImpact = 0.0;
                pair.dynamic = false;
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
                pair.timeOfImpact = 0.0;
                pair.dynamic = false;
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
                pair.timeOfImpact = 0.0;
                pair.dynamic = false;
                pairs.push_back(pair);
            }
        }
    }
    
    return pairs;
}

std::vector<PhysicsSystem::CollisionPair> PhysicsSystem::DetectSweptCollisionPairs(double dt) {
    std::vector<CollisionPair> pairs;
    if (dt <= 0.0) {
        return pairs;
    }

    auto boxColliders = entityManager_->GetAllWith<BoxCollider>();
    for (size_t i = 0; i < boxColliders.size(); ++i) {
        for (size_t j = i + 1; j < boxColliders.size(); ++j) {
            auto [entityA, colliderA] = boxColliders[i];
            auto [entityB, colliderB] = boxColliders[j];

            if (!colliderA->isEnabled || !colliderB->isEnabled) continue;
            if (!(colliderA->collisionLayer & colliderB->collisionMask)) continue;
            if (!(colliderB->collisionLayer & colliderA->collisionMask)) continue;

            auto* posA = entityManager_->GetComponent<Position>(entityA);
            auto* posB = entityManager_->GetComponent<Position>(entityB);
            if (!posA || !posB) continue;

            const auto* velA = entityManager_->GetComponent<Velocity>(entityA);
            const auto* velB = entityManager_->GetComponent<Velocity>(entityB);

            if (!velA && !velB) {
                continue;  // No relative motion
            }

            CollisionPair pair;
            if (ComputeSweptAABB(*colliderA, *posA, velA, *colliderB, *posB, velB, dt, pair)) {
                pair.entityA = entityA;
                pair.entityB = entityB;
                pair.dynamic = true;
                pairs.push_back(pair);
            }
        }
    }

    return pairs;
}

bool PhysicsSystem::ComputeSweptAABB(const BoxCollider& a, const Position& posA,
                                     const Velocity* velA,
                                     const BoxCollider& b, const Position& posB,
                                     const Velocity* velB, double dt,
                                     CollisionPair& result) {
    const double velAx = velA ? velA->vx : 0.0;
    const double velAy = velA ? velA->vy : 0.0;
    const double velAz = velA ? velA->vz : 0.0;
    const double velBx = velB ? velB->vx : 0.0;
    const double velBy = velB ? velB->vy : 0.0;
    const double velBz = velB ? velB->vz : 0.0;

    const double relVelX = velBx - velAx;
    const double relVelY = velBy - velAy;
    const double relVelZ = velBz - velAz;

    if (std::abs(relVelX) < 1e-8 && std::abs(relVelY) < 1e-8 && std::abs(relVelZ) < 1e-8) {
        return false;
    }

    const double aMinX = posA.x + a.offsetX - a.width * 0.5;
    const double aMaxX = posA.x + a.offsetX + a.width * 0.5;
    const double aMinY = posA.y + a.offsetY - a.height * 0.5;
    const double aMaxY = posA.y + a.offsetY + a.height * 0.5;
    const double aMinZ = posA.z + a.offsetZ - a.depth * 0.5;
    const double aMaxZ = posA.z + a.offsetZ + a.depth * 0.5;

    const double bMinX = posB.x + b.offsetX - b.width * 0.5;
    const double bMaxX = posB.x + b.offsetX + b.width * 0.5;
    const double bMinY = posB.y + b.offsetY - b.height * 0.5;
    const double bMaxY = posB.y + b.offsetY + b.height * 0.5;
    const double bMinZ = posB.z + b.offsetZ - b.depth * 0.5;
    const double bMaxZ = posB.z + b.offsetZ + b.depth * 0.5;

    auto computeAxis = [](double minA, double maxA, double minB, double maxB, double vel) {
        double entry, exit;
        if (vel > 0.0) {
            entry = (minA - maxB) / vel;
            exit = (maxA - minB) / vel;
        } else if (vel < 0.0) {
            entry = (maxA - minB) / vel;
            exit = (minA - maxB) / vel;
        } else {
            if (maxB < minA || minB > maxA) {
                entry = std::numeric_limits<double>::infinity();
                exit = -std::numeric_limits<double>::infinity();
            } else {
                entry = -std::numeric_limits<double>::infinity();
                exit = std::numeric_limits<double>::infinity();
            }
        }
        return std::pair<double, double>(entry, exit);
    };

    auto [entryX, exitX] = computeAxis(aMinX, aMaxX, bMinX, bMaxX, relVelX);
    auto [entryY, exitY] = computeAxis(aMinY, aMaxY, bMinY, bMaxY, relVelY);
    auto [entryZ, exitZ] = computeAxis(aMinZ, aMaxZ, bMinZ, bMaxZ, relVelZ);

    double entryTime = std::max({entryX, entryY, entryZ});
    double exitTime = std::min({exitX, exitY, exitZ});

    if (entryTime > exitTime) {
        return false;
    }

    if (exitTime < 0.0) {
        return false;
    }

    if (entryTime > dt) {
        return false;
    }

    if (entryTime < 0.0) {
        // Already overlapping or touching; handled by static detection
        return false;
    }

    double normalX = 0.0;
    double normalY = 0.0;
    double normalZ = 0.0;

    if (entryTime == entryX) {
        normalX = (relVelX > 0.0) ? -1.0 : 1.0;
    } else if (entryTime == entryY) {
        normalY = (relVelY > 0.0) ? -1.0 : 1.0;
    } else {
        normalZ = (relVelZ > 0.0) ? -1.0 : 1.0;
    }

    double clampedToi = Clamp(entryTime / dt, 0.0, 1.0);

    double hitPosAx = posA.x + velAx * entryTime;
    double hitPosAy = posA.y + velAy * entryTime;
    double hitPosAz = posA.z + velAz * entryTime;
    double hitPosBx = posB.x + velBx * entryTime;
    double hitPosBy = posB.y + velBy * entryTime;
    double hitPosBz = posB.z + velBz * entryTime;

    result.normalX = normalX;
    result.normalY = normalY;
    result.normalZ = normalZ;
    result.penetration = 0.0;
    result.timeOfImpact = clampedToi;
    result.dynamic = true;
    result.contactX = (hitPosAx + hitPosBx) * 0.5;
    result.contactY = (hitPosAy + hitPosBy) * 0.5;
    result.contactZ = (hitPosAz + hitPosBz) * 0.5;

    return true;
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
    if (currentCollisions_.empty()) {
        return;
    }

    std::sort(currentCollisions_.begin(), currentCollisions_.end(),
              [](const CollisionPair& lhs, const CollisionPair& rhs) {
                  return lhs.timeOfImpact < rhs.timeOfImpact;
              });

    for (const auto& pair : currentCollisions_) {
        auto* colliderA = entityManager_->GetComponent<Collider>(pair.entityA);
        auto* colliderB = entityManager_->GetComponent<Collider>(pair.entityB);

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
    auto* posA = entityManager_->GetComponent<Position>(pair.entityA);
    auto* posB = entityManager_->GetComponent<Position>(pair.entityB);

    CollisionPair workingPair = pair;

    if (pair.dynamic && dt > 0.0) {
        double rollback = (1.0 - Clamp(pair.timeOfImpact, 0.0, 1.0)) * dt;
        if (rollback > 0.0) {
            if (posA && velA && rbA && !rbA->isKinematic) {
                posA->x -= velA->vx * rollback;
                posA->y -= velA->vy * rollback;
                posA->z -= velA->vz * rollback;
            }
            if (posB && velB && rbB && !rbB->isKinematic) {
                posB->x -= velB->vx * rollback;
                posB->y -= velB->vy * rollback;
                posB->z -= velB->vz * rollback;
            }
        }

        if (posA && posB) {
            if (auto* boxA = entityManager_->GetComponent<BoxCollider>(pair.entityA)) {
                if (auto* boxB = entityManager_->GetComponent<BoxCollider>(pair.entityB)) {
                    CollisionPair recalculated;
                    if (CheckBoxBox(*boxA, *posA, *boxB, *posB, recalculated)) {
                        recalculated.entityA = pair.entityA;
                        recalculated.entityB = pair.entityB;
                        recalculated.timeOfImpact = pair.timeOfImpact;
                        recalculated.dynamic = pair.dynamic;
                        workingPair = recalculated;
                    }
                }
            }
        }
    }

    SeparateColliders(workingPair.entityA, workingPair.entityB,
                      workingPair.normalX, workingPair.normalY, workingPair.normalZ,
                      workingPair.penetration);

    if ((rbA && rbA->isKinematic) && (rbB && rbB->isKinematic)) return;

    const double velAx = velA ? velA->vx : 0.0;
    const double velAy = velA ? velA->vy : 0.0;
    const double velAz = velA ? velA->vz : 0.0;
    const double velBx = velB ? velB->vx : 0.0;
    const double velBy = velB ? velB->vy : 0.0;
    const double velBz = velB ? velB->vz : 0.0;

    double relVelX = velBx - velAx;
    double relVelY = velBy - velAy;
    double relVelZ = velBz - velAz;

    double velAlongNormal = DotProduct(relVelX, relVelY, relVelZ,
                                       workingPair.normalX, workingPair.normalY, workingPair.normalZ);

    if (velAlongNormal > 0.0) return;

    double restitution = 0.5;

    auto* colliderA = entityManager_->GetComponent<Collider>(pair.entityA);
    auto* colliderB = entityManager_->GetComponent<Collider>(pair.entityB);
    auto* materialA = entityManager_->GetComponent<PhysicsMaterial>(pair.entityA);
    auto* materialB = entityManager_->GetComponent<PhysicsMaterial>(pair.entityB);

    if (materialA) {
        restitution = materialA->restitution;
    }
    if (materialB) {
        restitution = materialA ? std::min(restitution, materialB->restitution) : materialB->restitution;
    }

    if (colliderA && colliderA->materialRestitution >= 0.0) {
        restitution = colliderA->materialRestitution;
    }
    if (colliderB && colliderB->materialRestitution >= 0.0) {
        restitution = std::min(restitution, colliderB->materialRestitution);
    }

    if (rbA && !rbB) {
        restitution = rbA->restitution;
    } else if (!rbA && rbB) {
        restitution = rbB->restitution;
    } else if (rbA && rbB) {
        restitution = std::min(rbA->restitution, rbB->restitution);
    }

    double invMassA = (rbA && !rbA->isKinematic) ? rbA->inverseMass : 0.0;
    double invMassB = (rbB && !rbB->isKinematic) ? rbB->inverseMass : 0.0;
    double invMassSum = invMassA + invMassB;

    if (invMassSum <= 0.0) {
        return;
    }

    double j = -(1.0 + restitution) * velAlongNormal;
    j /= invMassSum;

    const double nx = workingPair.normalX;
    const double ny = workingPair.normalY;
    const double nz = workingPair.normalZ;

    if (rbA && !rbA->isKinematic && velA) {
        velA->vx -= j * invMassA * nx;
        velA->vy -= j * invMassA * ny;
        velA->vz -= j * invMassA * nz;
        if (auto* accumA = entityManager_->GetComponent<ForceAccumulator>(pair.entityA)) {
            accumA->accumulatedImpulseX -= j * invMassA * nx;
            accumA->accumulatedImpulseY -= j * invMassA * ny;
            accumA->accumulatedImpulseZ -= j * invMassA * nz;
        }
    }

    if (rbB && !rbB->isKinematic && velB) {
        velB->vx += j * invMassB * nx;
        velB->vy += j * invMassB * ny;
        velB->vz += j * invMassB * nz;
        if (auto* accumB = entityManager_->GetComponent<ForceAccumulator>(pair.entityB)) {
            accumB->accumulatedImpulseX += j * invMassB * nx;
            accumB->accumulatedImpulseY += j * invMassB * ny;
            accumB->accumulatedImpulseZ += j * invMassB * nz;
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

    updateGrounding(pair.entityA, -nz);
    updateGrounding(pair.entityB, nz);
}

void PhysicsSystem::SeparateColliders(unsigned int entityA, unsigned int entityB,
                                       double normalX, double normalY, double normalZ,
                                       double penetration) {
    auto* rbA = entityManager_->GetComponent<RigidBody>(entityA);
    auto* rbB = entityManager_->GetComponent<RigidBody>(entityB);
    auto* posA = entityManager_->GetComponent<Position>(entityA);
    auto* posB = entityManager_->GetComponent<Position>(entityB);

    if (!posA || !posB) return;
    if (penetration <= 0.0) return;

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

    auto accumulators = entityManager_->GetAllWith<ForceAccumulator>();
    for (auto [entity, accumulator] : accumulators) {
        if (accumulator) {
            accumulator->Clear();
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

    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(entity)) {
        accum->accumulatedForceX += fx;
        accum->accumulatedForceY += fy;
        accum->accumulatedForceZ += fz;
    }
}

void PhysicsSystem::ApplyImpulse(unsigned int entity, double ix, double iy, double iz) {
    if (!entityManager_->IsAlive(entity)) return;

    auto& force = entityManager_->EmplaceComponent<Force>(entity);
    force.fx = ix;
    force.fy = iy;
    force.fz = iz;
    force.mode = Force::Mode::Impulse;
    force.lifetime = 0.0;

    if (auto* accum = entityManager_->GetComponent<ForceAccumulator>(entity)) {
        accum->accumulatedImpulseX += ix;
        accum->accumulatedImpulseY += iy;
        accum->accumulatedImpulseZ += iz;
    }
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
