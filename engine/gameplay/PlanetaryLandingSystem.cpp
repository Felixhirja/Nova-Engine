#include "PlanetaryLandingSystem.h"
#include "../ecs/Components.h"
#include "../MathUtils.h"
#include <cmath>
#include <algorithm>

namespace Nova {

// PlanetaryLandingSystem Implementation

void PlanetaryLandingSystem::Update(EntityManager& em, double deltaTime) {
    UpdateAtmosphericEntry(em, deltaTime);
    UpdateHeatShields(em, deltaTime);
    UpdateLandingGear(em, deltaTime);
    CheckLandingConditions(em, deltaTime);
}

void PlanetaryLandingSystem::UpdateAtmosphericEntry(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<PlanetaryAtmosphereComponent, Velocity, GravityWellComponent>();
    
    for (int entityId : entities) {
        auto* atmo = em.GetComponent<PlanetaryAtmosphereComponent>(entityId);
        auto* vel = em.GetComponent<Velocity>(entityId);
        auto* gravity = em.GetComponent<GravityWellComponent>(entityId);
        
        if (!atmo || !vel || !gravity) continue;
        
        // Calculate atmospheric effects
        float speed = glm::length(vel->velocity);
        float drag = CalculateAtmosphericDrag(*atmo, speed, gravity->altitude);
        
        // Apply drag force
        if (speed > 0.0f) {
            glm::vec3 dragForce = -glm::normalize(vel->velocity) * drag;
            vel->velocity += dragForce * static_cast<float>(deltaTime);
        }
        
        // Apply gravity
        vel->velocity.y -= gravity->surfaceGravity * static_cast<float>(deltaTime);
        
        // Check if in atmosphere
        gravity->inAtmosphere = gravity->altitude < 100000.0f; // 100km atmosphere boundary
        gravity->atmosphericDrag = drag;
    }
}

void PlanetaryLandingSystem::UpdateHeatShields(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<HeatShieldComponent, Velocity, AtmosphereComponent>();
    
    for (int entityId : entities) {
        auto* shield = em.GetComponent<HeatShieldComponent>(entityId);
        auto* vel = em.GetComponent<Velocity>(entityId);
        auto* atmo = em.GetComponent<AtmosphereComponent>(entityId);
        
        if (!shield || !vel || !atmo) continue;
        
        float speed = glm::length(vel->velocity);
        float heatLoad = CalculateHeatLoad(speed, atmo->density);
        
        if (shield->deployed) {
            // Heat up from atmospheric friction
            shield->currentHeat += heatLoad * static_cast<float>(deltaTime);
            
            // Ablative cooling
            if (shield->currentHeat > shield->maxHeat * 0.8f) {
                shield->ablativeThickness -= 0.01f * static_cast<float>(deltaTime);
                shield->currentHeat = std::min(shield->currentHeat, shield->maxHeat);
            }
            
            // Check for shield failure
            if (shield->ablativeThickness <= 0.0f) {
                shield->damaged = true;
                shield->integrity = 0.0f;
            } else {
                shield->integrity = (shield->ablativeThickness / 1.0f) * 100.0f;
            }
        } else {
            // Cool down when not deployed
            shield->currentHeat = std::max(300.0f, shield->currentHeat - shield->coolingRate * static_cast<float>(deltaTime));
        }
    }
}

void PlanetaryLandingSystem::UpdateLandingGear(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<LandingGearComponent>();
    
    for (int entityId : entities) {
        auto* gear = em.GetComponent<LandingGearComponent>(entityId);
        if (!gear) continue;
        
        // Update deployment animation
        if (gear->deployed && gear->currentDeployProgress < 1.0f) {
            gear->currentDeployProgress += (1.0f / gear->deployTime) * static_cast<float>(deltaTime);
            gear->currentDeployProgress = std::min(1.0f, gear->currentDeployProgress);
        } else if (!gear->deployed && gear->currentDeployProgress > 0.0f) {
            gear->currentDeployProgress -= (1.0f / gear->deployTime) * static_cast<float>(deltaTime);
            gear->currentDeployProgress = std::max(0.0f, gear->currentDeployProgress);
        }
        
        // Lock when fully deployed and on ground
        if (gear->deployed && gear->currentDeployProgress >= 1.0f && gear->onGround) {
            gear->locked = true;
        } else {
            gear->locked = false;
        }
    }
}

void PlanetaryLandingSystem::CheckLandingConditions(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<LandingGearComponent, Velocity, Position>();
    
    for (int entityId : entities) {
        auto* gear = em.GetComponent<LandingGearComponent>(entityId);
        auto* vel = em.GetComponent<Velocity>(entityId);
        auto* pos = em.GetComponent<Position>(entityId);
        
        if (!gear || !vel || !pos) continue;
        
        // Simple ground detection (y = 0 is ground level)
        if (pos->position.y <= gear->groundClearance && gear->deployed) {
            gear->onGround = true;
            
            // Check landing safety
            glm::vec3 landingVel = vel->velocity;
            float verticalSpeed = std::abs(landingVel.y);
            
            if (verticalSpeed > gear->maxLandingSpeed) {
                // Hard landing - damage calculation would go here
                vel->velocity *= 0.1f; // Absorb most velocity
            } else {
                // Safe landing
                vel->velocity = glm::vec3(0, 0, 0);
                pos->position.y = gear->groundClearance;
            }
        } else {
            gear->onGround = false;
        }
    }
}

float PlanetaryLandingSystem::CalculateAtmosphericDrag(const AtmosphereComponent& atmo, float velocity, float altitude) {
    // Simplified drag calculation: Fd = 0.5 * ρ * v² * Cd * A
    const float dragCoefficient = 0.5f;
    const float crossSectionalArea = 10.0f; // m²
    
    // Density decreases with altitude
    float densityFactor = std::exp(-altitude / 8500.0f); // Scale height ~8.5km
    float effectiveDensity = atmo.density * densityFactor;
    
    return 0.5f * effectiveDensity * velocity * velocity * dragCoefficient * crossSectionalArea;
}

float PlanetaryLandingSystem::CalculateHeatLoad(float velocity, float density) {
    // Simplified heating: Q = 0.5 * ρ * v³
    return 0.5f * density * velocity * velocity * velocity * 0.0001f; // Scale factor
}

bool PlanetaryLandingSystem::IsLandingSafe(const LandingGearComponent& gear, const glm::vec3& velocity, const glm::vec3& surfaceNormal) {
    float verticalSpeed = std::abs(glm::dot(velocity, surfaceNormal));
    return verticalSpeed <= gear.maxLandingSpeed && gear.deployed;
}

// EVASystem Implementation

void EVASystem::Update(EntityManager& em, double deltaTime) {
    UpdateOxygenConsumption(em, deltaTime);
    UpdateSuitIntegrity(em, deltaTime);
    UpdateJetpack(em, deltaTime);
    CheckEnvironmentalDamage(em, deltaTime);
}

void EVASystem::UpdateOxygenConsumption(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<EVASuitComponent>();
    
    for (int entityId : entities) {
        auto* suit = em.GetComponent<EVASuitComponent>(entityId);
        if (!suit || !suit->equipped || !suit->helmetSealed) continue;
        
        // Consume oxygen
        float consumption = suit->oxygenConsumptionRate * static_cast<float>(deltaTime);
        suit->oxygenRemaining = std::max(0.0f, suit->oxygenRemaining - consumption);
        
        // Warning system could be triggered here when oxygen is low
        if (suit->oxygenRemaining < 600.0f) { // 10 minutes remaining
            // Trigger low oxygen warning
        }
        
        // Life support failure when oxygen runs out
        if (suit->oxygenRemaining <= 0.0f) {
            suit->lifeSupportActive = false;
        }
    }
}

void EVASystem::UpdateSuitIntegrity(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<EVASuitComponent, AtmosphereComponent>();
    
    for (int entityId : entities) {
        auto* suit = em.GetComponent<EVASuitComponent>(entityId);
        auto* atmo = em.GetComponent<AtmosphereComponent>(entityId);
        
        if (!suit || !atmo) continue;
        
        // Integrity degradation from environmental factors
        if (atmo->toxicity > 0.5f || atmo->radiationLevel > 0.1f) {
            float degradation = atmo->toxicity * 0.1f + atmo->radiationLevel * 0.5f;
            suit->suitIntegrity -= degradation * static_cast<float>(deltaTime);
            suit->suitIntegrity = std::max(0.0f, suit->suitIntegrity);
        }
        
        // Critical failure
        if (suit->suitIntegrity < 20.0f) {
            suit->oxygenConsumptionRate *= 2.0f; // Leaks increase consumption
        }
    }
}

void EVASystem::UpdateJetpack(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<EVASuitComponent, Velocity>();
    
    for (int entityId : entities) {
        auto* suit = em.GetComponent<EVASuitComponent>(entityId);
        auto* vel = em.GetComponent<Velocity>(entityId);
        
        if (!suit || !vel) continue;
        
        // Jetpack fuel regenerates slowly when not in use
        if (suit->jetpackFuel < 100.0f) {
            suit->jetpackFuel += 5.0f * static_cast<float>(deltaTime);
            suit->jetpackFuel = std::min(100.0f, suit->jetpackFuel);
        }
    }
}

void EVASystem::CheckEnvironmentalDamage(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<EVASuitComponent, AtmosphereComponent>();
    
    for (int entityId : entities) {
        auto* suit = em.GetComponent<EVASuitComponent>(entityId);
        auto* atmo = em.GetComponent<AtmosphereComponent>(entityId);
        
        if (!suit || !atmo) continue;
        
        // Temperature regulation
        float tempDiff = std::abs(suit->temperature - atmo->temperature);
        if (tempDiff > 100.0f) {
            suit->suitIntegrity -= tempDiff * 0.001f * static_cast<float>(deltaTime);
        }
        
        // Radiation damage
        if (atmo->radiationLevel > suit->radiationShielding) {
            float radiationDamage = (atmo->radiationLevel - suit->radiationShielding) * 10.0f;
            suit->suitIntegrity -= radiationDamage * static_cast<float>(deltaTime);
        }
    }
}

// SurfaceVehicleSystem Implementation

void SurfaceVehicleSystem::Update(EntityManager& em, double deltaTime) {
    UpdateVehiclePhysics(em, deltaTime);
    UpdateFuelConsumption(em, deltaTime);
    HandleTerrainInteraction(em, deltaTime);
}

void SurfaceVehicleSystem::UpdateVehiclePhysics(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<SurfaceVehicleComponent, Velocity>();
    
    for (int entityId : entities) {
        auto* vehicle = em.GetComponent<SurfaceVehicleComponent>(entityId);
        auto* vel = em.GetComponent<Velocity>(entityId);
        
        if (!vehicle || !vel || !vehicle->active) continue;
        
        // Apply friction
        float friction = 0.95f;
        vel->velocity *= friction;
        
        // Max speed limiter
        float speed = glm::length(vel->velocity);
        if (speed > vehicle->maxSpeed) {
            vel->velocity = glm::normalize(vel->velocity) * vehicle->maxSpeed;
        }
    }
}

void SurfaceVehicleSystem::UpdateFuelConsumption(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<SurfaceVehicleComponent>();
    
    for (int entityId : entities) {
        auto* vehicle = em.GetComponent<SurfaceVehicleComponent>(entityId);
        if (!vehicle || !vehicle->active) continue;
        
        vehicle->fuel -= vehicle->fuelConsumption * static_cast<float>(deltaTime);
        vehicle->fuel = std::max(0.0f, vehicle->fuel);
        
        if (vehicle->fuel <= 0.0f) {
            vehicle->active = false;
        }
    }
}

void SurfaceVehicleSystem::HandleTerrainInteraction(EntityManager& em, double deltaTime) {
    // Terrain interaction would integrate with a heightmap or collision system
    // Placeholder for future terrain system integration
}

// WeatherSystem Implementation

void WeatherSystem::Update(EntityManager& em, double deltaTime) {
    UpdateWeatherPatterns(em, deltaTime);
    ApplyWeatherEffects(em, deltaTime);
    GenerateWeatherEvents(em, deltaTime);
}

void WeatherSystem::UpdateWeatherPatterns(EntityManager& em, double deltaTime) {
    weatherTransitionTimer += static_cast<float>(deltaTime);
    
    auto entities = em.GetEntitiesWithComponents<WeatherComponent>();
    
    for (int entityId : entities) {
        auto* weather = em.GetComponent<WeatherComponent>(entityId);
        if (!weather) continue;
        
        // Update wind
        float windVariation = std::sin(weatherTransitionTimer * 0.1f) * 5.0f;
        weather->windVector.x = std::cos(weatherTransitionTimer * 0.05f) * (10.0f + windVariation);
        weather->windVector.z = std::sin(weatherTransitionTimer * 0.05f) * (10.0f + windVariation);
        
        // Update visibility based on weather type
        switch (weather->currentWeather) {
            case WeatherComponent::WeatherType::Clear:
                weather->visibility = 10000.0f;
                weather->intensity = 0.0f;
                break;
            case WeatherComponent::WeatherType::Fog:
                weather->visibility = 100.0f;
                weather->intensity = 0.8f;
                break;
            case WeatherComponent::WeatherType::Storm:
                weather->visibility = 500.0f;
                weather->intensity = 0.9f;
                weather->hazardous = true;
                break;
            case WeatherComponent::WeatherType::Extreme:
                weather->visibility = 50.0f;
                weather->intensity = 1.0f;
                weather->hazardous = true;
                break;
            default:
                break;
        }
    }
}

void WeatherSystem::ApplyWeatherEffects(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<WeatherComponent, Velocity>();
    
    for (int entityId : entities) {
        auto* weather = em.GetComponent<WeatherComponent>(entityId);
        auto* vel = em.GetComponent<Velocity>(entityId);
        
        if (!weather || !vel) continue;
        
        // Apply wind force
        glm::vec3 windForce = weather->windVector * weather->intensity * static_cast<float>(deltaTime);
        vel->velocity += windForce * 0.1f;
    }
}

void WeatherSystem::GenerateWeatherEvents(EntityManager& em, double deltaTime) {
    if (weatherTransitionTimer >= minTransitionTime) {
        weatherTransitionTimer = 0.0f;
        
        // Procedurally transition weather
        // Implementation would include probability-based weather changes
    }
}

// DayNightCycleSystem Implementation

void DayNightCycleSystem::Update(EntityManager& em, double deltaTime) {
    UpdateSunPosition(em, deltaTime);
    UpdateAmbientLighting(em, deltaTime);
}

void DayNightCycleSystem::UpdateSunPosition(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<DayNightCycleComponent>();
    
    for (int entityId : entities) {
        auto* cycle = em.GetComponent<DayNightCycleComponent>(entityId);
        if (!cycle) continue;
        
        // Advance time
        cycle->currentTime += static_cast<float>(deltaTime);
        if (cycle->currentTime >= cycle->dayLength) {
            cycle->currentTime -= cycle->dayLength;
        }
        
        // Calculate sun angle (0 = midnight, π = noon)
        float timeRatio = cycle->currentTime / cycle->dayLength;
        cycle->sunAngle = timeRatio * 2.0f * 3.14159f;
        
        // Update sun direction
        cycle->sunDirection.x = std::cos(cycle->sunAngle);
        cycle->sunDirection.y = std::sin(cycle->sunAngle);
        cycle->sunDirection.z = 0.0f;
        cycle->sunDirection = glm::normalize(cycle->sunDirection);
        
        // Determine if daytime
        cycle->isDaytime = (cycle->sunAngle > 0 && cycle->sunAngle < 3.14159f);
    }
}

void DayNightCycleSystem::UpdateAmbientLighting(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<DayNightCycleComponent>();
    
    for (int entityId : entities) {
        auto* cycle = em.GetComponent<DayNightCycleComponent>(entityId);
        if (!cycle) continue;
        
        // Calculate ambient light based on sun position
        float sunHeight = cycle->sunDirection.y;
        
        if (sunHeight > 0.0f) {
            // Daytime
            cycle->ambientLight = 0.3f + 0.7f * sunHeight;
        } else {
            // Nighttime
            cycle->ambientLight = 0.1f;
        }
        
        // Smooth twilight transitions
        float twilightFactor = std::abs(sunHeight);
        if (twilightFactor < 0.1f) {
            cycle->ambientLight *= (twilightFactor / 0.1f);
        }
    }
}

// ResourceScanningSystem Implementation

void ResourceScanningSystem::Update(EntityManager& em, double deltaTime) {
    PerformScans(em, deltaTime);
    DetectResources(em, deltaTime);
    UpdateScanProgress(em, deltaTime);
}

void ResourceScanningSystem::PerformScans(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<SurfaceScannerComponent, Position>();
    
    for (int entityId : entities) {
        auto* scanner = em.GetComponent<SurfaceScannerComponent>(entityId);
        auto* pos = em.GetComponent<Position>(entityId);
        
        if (!scanner || !pos || !scanner->scanning) continue;
        
        // Progress the scan
        scanner->scanProgress += (1.0f / 5.0f) * static_cast<float>(deltaTime); // 5 second scan
        scanner->scanProgress = std::min(1.0f, scanner->scanProgress);
        
        if (scanner->scanProgress >= 1.0f) {
            scanner->scanning = false;
            scanner->scanProgress = 0.0f;
        }
    }
}

void ResourceScanningSystem::DetectResources(EntityManager& em, double deltaTime) {
    auto scanners = em.GetEntitiesWithComponents<SurfaceScannerComponent, Position>();
    auto deposits = em.GetEntitiesWithComponents<ResourceDepositComponent, Position>();
    
    for (int scannerId : scanners) {
        auto* scanner = em.GetComponent<SurfaceScannerComponent>(scannerId);
        auto* scanPos = em.GetComponent<Position>(scannerId);
        
        if (!scanner || !scanPos) continue;
        
        scanner->detectedResources.clear();
        
        for (int depositId : deposits) {
            auto* deposit = em.GetComponent<ResourceDepositComponent>(depositId);
            auto* depPos = em.GetComponent<Position>(depositId);
            
            if (!deposit || !depPos) continue;
            
            float distance = glm::distance(scanPos->position, depPos->position);
            
            if (distance <= scanner->scanRange) {
                scanner->detectedResources.push_back(depositId);
                deposit->discovered = true;
            }
        }
    }
}

void ResourceScanningSystem::UpdateScanProgress(EntityManager& em, double deltaTime) {
    // Additional scan processing logic
}

// MiningSystem Implementation

void MiningSystem::Update(EntityManager& em, double deltaTime) {
    ProcessMining(em, deltaTime);
    UpdateMiningEquipment(em, deltaTime);
    HandleHeatBuildup(em, deltaTime);
}

void MiningSystem::ProcessMining(EntityManager& em, double deltaTime) {
    auto miners = em.GetEntitiesWithComponents<MiningEquipmentComponent, Position>();
    auto deposits = em.GetEntitiesWithComponents<ResourceDepositComponent, Position>();
    
    for (int minerId : miners) {
        auto* equipment = em.GetComponent<MiningEquipmentComponent>(minerId);
        auto* minerPos = em.GetComponent<Position>(minerId);
        
        if (!equipment || !minerPos || !equipment->active) continue;
        
        for (int depositId : deposits) {
            auto* deposit = em.GetComponent<ResourceDepositComponent>(depositId);
            auto* depPos = em.GetComponent<Position>(depositId);
            
            if (!deposit || !depPos) continue;
            
            float distance = glm::distance(minerPos->position, depPos->position);
            
            if (distance <= equipment->range && deposit->quantity > 0.0f) {
                // Mine resources
                float mined = equipment->miningRate * equipment->efficiency * static_cast<float>(deltaTime);
                mined = std::min(mined, deposit->quantity);
                
                deposit->quantity -= mined;
                DepositResources(em, minerId, mined);
                
                // Generate heat
                equipment->heatGeneration += mined * 0.1f;
            }
        }
    }
}

void MiningSystem::UpdateMiningEquipment(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<MiningEquipmentComponent>();
    
    for (int entityId : entities) {
        auto* equipment = em.GetComponent<MiningEquipmentComponent>(entityId);
        if (!equipment) continue;
        
        // Power drain
        if (equipment->active && equipment->power > 0.0f) {
            equipment->power -= 1.0f * static_cast<float>(deltaTime);
            equipment->power = std::max(0.0f, equipment->power);
        } else {
            equipment->power += 5.0f * static_cast<float>(deltaTime);
            equipment->power = std::min(100.0f, equipment->power);
        }
        
        if (equipment->power <= 0.0f) {
            equipment->active = false;
        }
    }
}

void MiningSystem::HandleHeatBuildup(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<MiningEquipmentComponent>();
    
    for (int entityId : entities) {
        auto* equipment = em.GetComponent<MiningEquipmentComponent>(entityId);
        if (!equipment) continue;
        
        // Cool down when not active
        if (!equipment->active) {
            equipment->heatGeneration -= 10.0f * static_cast<float>(deltaTime);
            equipment->heatGeneration = std::max(0.0f, equipment->heatGeneration);
        }
        
        // Overheat protection
        if (equipment->heatGeneration >= equipment->maxHeat) {
            equipment->active = false;
            equipment->efficiency *= 0.9f; // Damage from overheating
        }
    }
}

void MiningSystem::DepositResources(EntityManager& em, int entityId, float amount) {
    // Would integrate with cargo/inventory system
    // Placeholder for resource collection
}

// EnvironmentalHazardSystem Implementation

void EnvironmentalHazardSystem::Update(EntityManager& em, double deltaTime) {
    ApplyHazardDamage(em, deltaTime);
    CheckHazardProximity(em, deltaTime);
    UpdateHazardStates(em, deltaTime);
}

void EnvironmentalHazardSystem::ApplyHazardDamage(EntityManager& em, double deltaTime) {
    auto hazards = em.GetEntitiesWithComponents<EnvironmentalHazardComponent, Position>();
    auto entities = em.GetEntitiesWithComponents<Position, Health>();
    
    for (int hazardId : hazards) {
        auto* hazard = em.GetComponent<EnvironmentalHazardComponent>(hazardId);
        auto* hazardPos = em.GetComponent<Position>(hazardId);
        
        if (!hazard || !hazardPos || !hazard->active) continue;
        
        for (int entityId : entities) {
            auto* pos = em.GetComponent<Position>(entityId);
            auto* health = em.GetComponent<Health>(entityId);
            
            if (!pos || !health) continue;
            
            float distance = glm::distance(pos->position, hazardPos->position);
            
            if (distance <= hazard->radius) {
                float damageFactor = 1.0f - (distance / hazard->radius);
                float damage = hazard->damageRate * hazard->intensity * damageFactor * static_cast<float>(deltaTime);
                
                health->currentHealth -= damage;
            }
        }
    }
}

void EnvironmentalHazardSystem::CheckHazardProximity(EntityManager& em, double deltaTime) {
    // Check for warning systems and HUD alerts
}

void EnvironmentalHazardSystem::UpdateHazardStates(EntityManager& em, double deltaTime) {
    auto entities = em.GetEntitiesWithComponents<EnvironmentalHazardComponent>();
    
    for (int entityId : entities) {
        auto* hazard = em.GetComponent<EnvironmentalHazardComponent>(entityId);
        if (!hazard) continue;
        
        // Update timed hazards
        if (hazard->duration > 0.0f) {
            hazard->duration -= static_cast<float>(deltaTime);
            if (hazard->duration <= 0.0f) {
                hazard->active = false;
            }
        }
    }
}

} // namespace Nova
