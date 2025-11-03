#include "CombatSystems.h"
#include "EntityManager.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// WEAPON FIRE SYSTEM
// ============================================================================

void WeaponFireSystem::Update(EntityManager& em, double dt) {
    // Update all weapon systems
    em.ForEach<WeaponSystem, Position>([&](ecs::EntityHandle entity, 
                                           WeaponSystem& weapon, 
                                           Position& pos) {
        // Update cooldowns
        if (weapon.currentCooldown > 0.0) {
            weapon.currentCooldown -= dt;
        }
        
        // Update reload
        if (weapon.isReloading) {
            weapon.reloadProgress += dt;
            if (weapon.reloadProgress >= weapon.reloadTime) {
                weapon.isReloading = false;
                weapon.reloadProgress = 0.0;
                weapon.ammo = weapon.maxAmmo;
            }
            return;
        }
        
        // Check if firing
        if (!weapon.isFiring || weapon.currentCooldown > 0.0) {
            return;
        }
        
        // Check ammo
        if (weapon.ammo == 0) {
            weapon.isReloading = true;
            weapon.reloadProgress = 0.0;
            return;
        }
        
        // Fire weapon based on type
        if (weapon.type == WeaponType::Laser || weapon.type == WeaponType::Beam) {
            ProcessEnergyWeapon(em, entity, weapon, dt);
        } else {
            ProcessBallisticWeapon(em, entity, weapon, dt);
        }
    });
    
    // Update missile launchers
    em.ForEach<MissileWeapon, Position>([&](ecs::EntityHandle entity,
                                            MissileWeapon& missile,
                                            Position& pos) {
        // Update reload
        if (missile.reloadProgress < missile.reloadTime) {
            missile.reloadProgress += dt;
            return;
        }
        
        // Update lock-on
        if (missile.lockedTarget.IsValid()) {
            missile.lockProgress += dt;
            if (missile.lockProgress >= missile.lockOnTime) {
                missile.isLocked = true;
            }
        } else {
            missile.lockProgress = 0.0;
            missile.isLocked = false;
        }
        
        // Fire if locked and requested
        if (missile.isLocked && missile.ammo > 0) {
            ProcessMissileWeapon(em, entity, missile, dt);
        }
    });
}

void WeaponFireSystem::ProcessEnergyWeapon(EntityManager& em, ecs::EntityHandle entity,
                                          WeaponSystem& weapon, double dt) {
    // Get ship position and orientation
    auto pos = em.GetComponent<Position>(entity);
    if (!pos) return;
    
    // Get hardpoint if available for direction
    double dirX = 1.0, dirY = 0.0, dirZ = 0.0;  // Default forward
    
    // Spawn projectile
    auto projectile = SpawnProjectile(em, entity, weapon,
                                     pos->x, pos->y, pos->z,
                                     dirX, dirY, dirZ);
    
    // Update weapon state
    weapon.currentCooldown = weapon.cooldown;
    if (weapon.ammo > 0) {
        weapon.ammo--;
    }
    
    // Add heat
    auto thermal = em.GetComponent<ThermalSystem>(entity);
    if (thermal) {
        thermal->heat += weapon.heatPerShot;
    }
}

void WeaponFireSystem::ProcessBallisticWeapon(EntityManager& em, ecs::EntityHandle entity,
                                              WeaponSystem& weapon, double dt) {
    auto pos = em.GetComponent<Position>(entity);
    if (!pos) return;
    
    double dirX = 1.0, dirY = 0.0, dirZ = 0.0;
    
    // Apply weapon spread
    std::random_device rd;
    std::mt19937 rng(rd());
    CombatUtils::ApplyWeaponSpread(dirX, dirY, dirZ, weapon.spread, rng);
    
    auto projectile = SpawnProjectile(em, entity, weapon,
                                     pos->x, pos->y, pos->z,
                                     dirX, dirY, dirZ);
    
    weapon.currentCooldown = weapon.cooldown;
    if (weapon.ammo > 0) {
        weapon.ammo--;
    }
}

void WeaponFireSystem::ProcessMissileWeapon(EntityManager& em, ecs::EntityHandle entity,
                                           MissileWeapon& missile, double dt) {
    if (!missile.isLocked || missile.ammo <= 0) return;
    
    auto proj = SpawnMissile(em, entity, missile, missile.lockedTarget);
    
    missile.ammo--;
    missile.reloadProgress = 0.0;
    missile.lockProgress = 0.0;
    missile.isLocked = false;
}

ecs::EntityHandle WeaponFireSystem::SpawnProjectile(EntityManager& em, 
                                                    ecs::EntityHandle owner,
                                                    const WeaponSystem& weapon,
                                                    double posX, double posY, double posZ,
                                                    double dirX, double dirY, double dirZ) {
    auto projectile = em.CreateEntity();
    
    // Position
    auto pos = std::make_shared<Position>(posX, posY, posZ);
    em.AddComponent<Position>(projectile, pos);
    
    // Velocity
    auto vel = std::make_shared<Velocity>(
        dirX * weapon.projectileSpeed,
        dirY * weapon.projectileSpeed,
        dirZ * weapon.projectileSpeed
    );
    em.AddComponent<Velocity>(projectile, vel);
    
    // Projectile data
    auto data = std::make_shared<ProjectileData>();
    data->owner = owner;
    data->weaponType = weapon.type;
    data->damageType = weapon.damageType;
    data->damage = weapon.baseDamage;
    data->armorPenetration = weapon.armorPenetration;
    data->shieldPenetration = weapon.shieldPenetration;
    data->speed = weapon.projectileSpeed;
    data->lifetime = weapon.projectileLifetime;
    data->isGuided = false;
    em.AddComponent<ProjectileData>(projectile, data);
    
    // Visual component
    auto draw = std::make_shared<DrawComponent>();
    draw->mode = DrawComponent::RenderMode::Billboard;
    draw->visible = true;
    draw->SetTint(1.0f, 0.3f, 0.1f);
    em.AddComponent<DrawComponent>(projectile, draw);
    
    return projectile;
}

ecs::EntityHandle WeaponFireSystem::SpawnMissile(EntityManager& em,
                                                 ecs::EntityHandle owner,
                                                 const MissileWeapon& launcher,
                                                 ecs::EntityHandle target) {
    auto ownerPos = em.GetComponent<Position>(owner);
    if (!ownerPos) return ecs::EntityHandle::Null();
    
    auto missile = em.CreateEntity();
    
    auto pos = std::make_shared<Position>(ownerPos->x, ownerPos->y, ownerPos->z);
    em.AddComponent<Position>(missile, pos);
    
    auto vel = std::make_shared<Velocity>(0, 0, launcher.missileSpeed);
    em.AddComponent<Velocity>(missile, vel);
    
    auto data = std::make_shared<ProjectileData>();
    data->owner = owner;
    data->weaponType = WeaponType::Missile;
    data->damageType = DamageType::Explosive;
    data->damage = launcher.missileDamage;
    data->speed = launcher.missileMaxSpeed;
    data->lifetime = launcher.missileLifetime;
    data->isGuided = true;
    data->target = target;
    data->turnRate = launcher.missileTurnRate;
    data->acceleration = launcher.missileAcceleration;
    data->armingRange = launcher.missileArmingRange;
    data->isExplosive = true;
    data->blastRadius = launcher.missileBlastRadius;
    em.AddComponent<ProjectileData>(missile, data);
    
    auto draw = std::make_shared<DrawComponent>();
    draw->mode = DrawComponent::RenderMode::Mesh3D;
    draw->visible = true;
    em.AddComponent<DrawComponent>(missile, draw);
    
    return missile;
}

// ============================================================================
// PROJECTILE SYSTEM
// ============================================================================

void ProjectileSystem::Update(EntityManager& em, double dt) {
    std::vector<ecs::EntityHandle> toDestroy;
    
    em.ForEach<ProjectileData, Position, Velocity>([&](ecs::EntityHandle entity,
                                                       ProjectileData& proj,
                                                       Position& pos,
                                                       Velocity& vel) {
        // Update lifetime
        proj.elapsed += dt;
        if (proj.elapsed >= proj.lifetime) {
            toDestroy.push_back(entity);
            return;
        }
        
        // Update guided projectiles
        if (proj.isGuided && proj.target.IsValid()) {
            UpdateGuidedProjectile(em, entity, proj, pos, vel, dt);
        }
        
        // Check for arming
        if (!proj.armed && proj.armingRange > 0.0) {
            auto ownerPos = em.GetComponent<Position>(proj.owner);
            if (ownerPos) {
                double dx = pos.x - ownerPos->x;
                double dy = pos.y - ownerPos->y;
                double dz = pos.z - ownerPos->z;
                double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                if (dist >= proj.armingRange) {
                    proj.armed = true;
                }
            }
        }
        
        // Check collisions
        double hitDist;
        if (CheckCollision(em, entity, pos, hitDist)) {
            toDestroy.push_back(entity);
        }
    });
    
    // Destroy expired/hit projectiles
    for (auto entity : toDestroy) {
        em.DestroyEntity(entity);
    }
}

void ProjectileSystem::UpdateGuidedProjectile(EntityManager& em, ecs::EntityHandle entity,
                                             ProjectileData& proj, Position& pos,
                                             Velocity& vel, double dt) {
    auto targetPos = em.GetComponent<Position>(proj.target);
    if (!targetPos) {
        proj.isGuided = false;
        return;
    }
    
    // Calculate direction to target
    double dx = targetPos->x - pos.x;
    double dy = targetPos->y - pos.y;
    double dz = targetPos->z - pos.z;
    double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    if (dist < 0.1) return;
    
    // Normalize target direction
    dx /= dist;
    dy /= dist;
    dz /= dist;
    
    // Current velocity direction
    double speed = std::sqrt(vel.vx*vel.vx + vel.vy*vel.vy + vel.vz*vel.vz);
    if (speed < 0.1) speed = proj.speed;
    
    double vdx = vel.vx / speed;
    double vdy = vel.vy / speed;
    double vdz = vel.vz / speed;
    
    // Steer towards target (limited by turn rate)
    double maxTurn = proj.turnRate * dt * (M_PI / 180.0);  // Convert to radians
    
    // Simple proportional guidance
    double newVX = vdx + (dx - vdx) * std::min(maxTurn, 1.0);
    double newVY = vdy + (dy - vdy) * std::min(maxTurn, 1.0);
    double newVZ = vdz + (dz - vdz) * std::min(maxTurn, 1.0);
    
    // Normalize and apply speed
    double newSpeed = std::sqrt(newVX*newVX + newVY*newVY + newVZ*newVZ);
    if (newSpeed > 0.01) {
        vel.vx = (newVX / newSpeed) * speed;
        vel.vy = (newVY / newSpeed) * speed;
        vel.vz = (newVZ / newSpeed) * speed;
    }
    
    // Apply acceleration
    if (proj.acceleration > 0.0) {
        double currentSpeed = std::sqrt(vel.vx*vel.vx + vel.vy*vel.vy + vel.vz*vel.vz);
        double newSpeed = std::min(currentSpeed + proj.acceleration * dt, proj.speed);
        if (currentSpeed > 0.01) {
            double scale = newSpeed / currentSpeed;
            vel.vx *= scale;
            vel.vy *= scale;
            vel.vz *= scale;
        }
    }
}

bool ProjectileSystem::CheckCollision(EntityManager& em, ecs::EntityHandle projectile,
                                     const Position& projPos, double& hitDist) {
    auto projData = em.GetComponent<ProjectileData>(projectile);
    if (!projData || !projData->armed) return false;
    
    // Check against all entities with position and health
    bool hit = false;
    ecs::EntityHandle hitTarget = ecs::EntityHandle::Null();
    double minDist = 10.0;  // Collision radius
    
    em.ForEach<Position, Health>([&](ecs::EntityHandle entity, Position& pos, Health& health) {
        if (entity == projectile || entity == projData->owner) return;
        
        double dx = pos.x - projPos.x;
        double dy = pos.y - projPos.y;
        double dz = pos.z - projPos.z;
        double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
        
        if (dist < minDist) {
            hit = true;
            hitTarget = entity;
            hitDist = dist;
            minDist = dist;
        }
    });
    
    if (hit && hitTarget.IsValid()) {
        ApplyDamage(em, projectile, hitTarget, *projData);
        
        // Handle explosive damage
        if (projData->isExplosive && projData->blastRadius > 0.0) {
            em.ForEach<Position, Health>([&](ecs::EntityHandle entity, Position& pos, Health& health) {
                if (entity == hitTarget) return;
                
                double dx = pos.x - projPos.x;
                double dy = pos.y - projPos.y;
                double dz = pos.z - projPos.z;
                double dist = std::sqrt(dx*dx + dy*dy + dz*dz);
                
                if (dist <= projData->blastRadius) {
                    double explosionDamage = CombatUtils::CalculateExplosionDamage(
                        projData->damage, dist, projData->blastRadius);
                    health.current -= explosionDamage * 0.5;  // Explosive splash does reduced damage
                }
            });
        }
    }
    
    return hit;
}

void ProjectileSystem::ApplyDamage(EntityManager& em, ecs::EntityHandle projectile,
                                  ecs::EntityHandle target, const ProjectileData& data) {
    // Apply to shields first
    auto shields = em.GetComponent<DirectionalShields>(target);
    double remainingDamage = data.damage;
    
    if (shields && shields->shieldsEnabled) {
        // Determine shield facing
        auto targetPos = em.GetComponent<Position>(target);
        auto projPos = em.GetComponent<Position>(projectile);
        
        if (targetPos && projPos) {
            double relX = projPos->x - targetPos->x;
            double relY = projPos->y - targetPos->y;
            double relZ = projPos->z - targetPos->z;
            
            ShieldFacing facing = CombatUtils::WorldToShieldFacing(relX, relY, relZ);
            
            auto& face = shields->faces[facing];
            double shieldDamage = data.damage * (1.0 - data.shieldPenetration);
            double absorbed = std::min(face.currentShields, shieldDamage);
            face.currentShields -= absorbed;
            remainingDamage -= absorbed;
            
            if (remainingDamage < 0.0) remainingDamage = 0.0;
        }
    }
    
    // Apply remaining damage to hull/armor
    auto hull = em.GetComponent<HullDamage>(target);
    if (hull) {
        // Check armor first
        if (hull->currentArmor > 0.0) {
            double armorDamage = remainingDamage * (1.0 - data.armorPenetration);
            double absorbed = std::min(hull->currentArmor, armorDamage);
            hull->currentArmor -= absorbed;
            remainingDamage -= absorbed;
            
            if (remainingDamage < 0.0) remainingDamage = 0.0;
        }
        
        // Apply to hull
        hull->currentHull -= remainingDamage;
    } else {
        // No hull component, apply to health directly
        auto health = em.GetComponent<Health>(target);
        if (health) {
            health->current -= remainingDamage;
        }
    }
    
    // Update statistics
    auto stats = em.GetComponent<CombatStatistics>(data.owner);
    if (stats) {
        stats->totalDamageDealt += data.damage;
        stats->shotsHit++;
    }
}

// ============================================================================
// DIRECTIONAL SHIELD SYSTEM
// ============================================================================

void DirectionalShieldSystem::Update(EntityManager& em, double dt) {
    double currentTime = 0.0;  // TODO: Get actual game time
    
    em.ForEach<DirectionalShields>([&](ecs::EntityHandle entity, DirectionalShields& shields) {
        if (!shields.shieldsEnabled) return;
        
        // Recharge shields
        RechargeShields(shields, dt, currentTime);
        
        // Handle shield rebalancing
        if (shields.canRebalance) {
            HandleShieldRebalancing(shields, dt);
        }
        
        // Update power draw
        double recharging = 0.0;
        for (auto& pair : shields.faces) {
            if (pair.second.currentShields < pair.second.maxShields) {
                recharging += pair.second.rechargeRate;
            }
        }
        shields.currentPowerDraw = recharging * 0.1 * shields.powerAllocation;
    });
}

void DirectionalShieldSystem::RechargeShields(DirectionalShields& shields, double dt, 
                                             double currentTime) {
    for (auto& pair : shields.faces) {
        auto& face = pair.second;
        
        // Check if in recharge delay
        if (currentTime - face.lastDamageTime < face.rechargeDelay) {
            continue;
        }
        
        // Check if overloaded
        if (face.overloaded) {
            face.overloadRecovery += dt;
            if (face.overloadRecovery >= 10.0) {  // 10 second recovery
                face.overloaded = false;
                face.overloadRecovery = 0.0;
            }
            continue;
        }
        
        // Recharge
        if (face.currentShields < face.maxShields) {
            double recharge = face.rechargeRate * dt * shields.globalRechargeMultiplier * 
                            shields.powerAllocation;
            face.currentShields = std::min(face.maxShields, face.currentShields + recharge);
        }
    }
}

void DirectionalShieldSystem::HandleShieldRebalancing(DirectionalShields& shields, double dt) {
    // Find weakest and strongest faces
    ShieldFacing weakest = ShieldFacing::Forward;
    ShieldFacing strongest = ShieldFacing::Forward;
    double minPercent = 1.0;
    double maxPercent = 0.0;
    
    for (const auto& pair : shields.faces) {
        double percent = pair.second.currentShields / pair.second.maxShields;
        if (percent < minPercent) {
            minPercent = percent;
            weakest = pair.first;
        }
        if (percent > maxPercent) {
            maxPercent = percent;
            strongest = pair.first;
        }
    }
    
    // Transfer shields if there's a significant difference
    if (maxPercent - minPercent > 0.3) {
        double transfer = shields.rebalanceRate * dt;
        auto& weakFace = shields.faces[weakest];
        auto& strongFace = shields.faces[strongest];
        
        double available = strongFace.currentShields - strongFace.maxShields * 0.5;
        if (available > 0.0) {
            double amount = std::min(transfer, available);
            amount = std::min(amount, weakFace.maxShields - weakFace.currentShields);
            
            strongFace.currentShields -= amount;
            weakFace.currentShields += amount;
        }
    }
}

ShieldFacing DirectionalShieldSystem::DetermineFacing(double impactX, double impactY, double impactZ,
                                                     double shipX, double shipY, double shipZ,
                                                     double shipYaw, double shipPitch, double shipRoll) {
    // Calculate relative position
    double relX = impactX - shipX;
    double relY = impactY - shipY;
    double relZ = impactZ - shipZ;
    
    return CombatUtils::WorldToShieldFacing(relX, relY, relZ);
}

// ============================================================================
// COMBAT UTILITIES
// ============================================================================

namespace CombatUtils {
    double CalculateEffectiveDamage(double baseDamage, double penetration,
                                   double armor, double armorEffectiveness) {
        double blocked = armor * armorEffectiveness * (1.0 - penetration);
        return std::max(0.0, baseDamage - blocked);
    }
    
    bool RollHitChance(double accuracy, double range, double optimalRange,
                      double maxRange, double targetSpeed, double jamming) {
        double baseChance = accuracy;
        
        // Range penalty
        if (range > optimalRange) {
            double rangeFactor = 1.0 - ((range - optimalRange) / (maxRange - optimalRange));
            baseChance *= std::max(0.1, rangeFactor);
        }
        
        // Target speed penalty
        baseChance *= std::max(0.3, 1.0 - (targetSpeed / 1000.0));
        
        // Jamming penalty
        baseChance *= (1.0 - jamming * 0.5);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> dis(0.0, 1.0);
        
        return dis(gen) < baseChance;
    }
    
    void ApplyWeaponSpread(double& dirX, double& dirY, double& dirZ,
                          double spreadDegrees, std::mt19937& rng) {
        std::uniform_real_distribution<> dis(-spreadDegrees, spreadDegrees);
        
        double yawSpread = dis(rng) * (M_PI / 180.0);
        double pitchSpread = dis(rng) * (M_PI / 180.0);
        
        // Apply rotation (simplified)
        double cosYaw = std::cos(yawSpread);
        double sinYaw = std::sin(yawSpread);
        double cosPitch = std::cos(pitchSpread);
        double sinPitch = std::sin(pitchSpread);
        
        double newX = dirX * cosYaw - dirZ * sinYaw;
        double newZ = dirX * sinYaw + dirZ * cosYaw;
        double newY = dirY * cosPitch + newZ * sinPitch;
        newZ = -dirY * sinPitch + newZ * cosPitch;
        
        dirX = newX;
        dirY = newY;
        dirZ = newZ;
        
        // Normalize
        double len = std::sqrt(dirX*dirX + dirY*dirY + dirZ*dirZ);
        if (len > 0.01) {
            dirX /= len;
            dirY /= len;
            dirZ /= len;
        }
    }
    
    bool CalculateInterceptPoint(double projSpeed,
                                double shooterX, double shooterY, double shooterZ,
                                double targetX, double targetY, double targetZ,
                                double targetVX, double targetVY, double targetVZ,
                                double& interceptX, double& interceptY, double& interceptZ,
                                double& timeToIntercept) {
        // Relative position
        double rx = targetX - shooterX;
        double ry = targetY - shooterY;
        double rz = targetZ - shooterZ;
        
        // Quadratic equation coefficients
        double a = targetVX*targetVX + targetVY*targetVY + targetVZ*targetVZ - projSpeed*projSpeed;
        double b = 2.0 * (rx*targetVX + ry*targetVY + rz*targetVZ);
        double c = rx*rx + ry*ry + rz*rz;
        
        double discriminant = b*b - 4*a*c;
        if (discriminant < 0.0) return false;
        
        double t1 = (-b + std::sqrt(discriminant)) / (2*a);
        double t2 = (-b - std::sqrt(discriminant)) / (2*a);
        
        timeToIntercept = (t1 > 0.0 && t1 < t2) ? t1 : t2;
        if (timeToIntercept < 0.0) return false;
        
        interceptX = targetX + targetVX * timeToIntercept;
        interceptY = targetY + targetVY * timeToIntercept;
        interceptZ = targetZ + targetVZ * timeToIntercept;
        
        return true;
    }
    
    double GetDamageTypeMultiplier(DamageType damage, bool vsShield) {
        if (vsShield) {
            switch (damage) {
                case DamageType::Energy: return 1.5;
                case DamageType::Kinetic: return 0.7;
                case DamageType::Electromagnetic: return 2.0;
                default: return 1.0;
            }
        } else {
            switch (damage) {
                case DamageType::Kinetic: return 1.5;
                case DamageType::Energy: return 0.8;
                case DamageType::Explosive: return 1.3;
                default: return 1.0;
            }
        }
    }
    
    ShieldFacing WorldToShieldFacing(double relX, double relY, double relZ) {
        // Determine primary direction
        double absX = std::abs(relX);
        double absY = std::abs(relY);
        double absZ = std::abs(relZ);
        
        if (absZ > absX && absZ > absY) {
            return relZ > 0.0 ? ShieldFacing::Forward : ShieldFacing::Aft;
        } else if (absX > absY) {
            return relX > 0.0 ? ShieldFacing::Starboard : ShieldFacing::Port;
        } else {
            return relY > 0.0 ? ShieldFacing::Dorsal : ShieldFacing::Ventral;
        }
    }
    
    double CalculateExplosionDamage(double baseDamage, double distance, double blastRadius) {
        if (distance >= blastRadius) return 0.0;
        double falloff = 1.0 - (distance / blastRadius);
        return baseDamage * falloff * falloff;  // Quadratic falloff
    }
}
