#pragma once

#include "../ecs/System.h"
#include "../ecs/EntityManager.h"
#include "../ecs/PlanetaryComponents.h"
#include <glm/glm.hpp>

namespace Nova {

class PlanetaryLandingSystem : public System {
public:
    PlanetaryLandingSystem() : System(SystemType::PlanetaryLanding) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void UpdateAtmosphericEntry(EntityManager& em, double deltaTime);
    void UpdateHeatShields(EntityManager& em, double deltaTime);
    void UpdateLandingGear(EntityManager& em, double deltaTime);
    void CheckLandingConditions(EntityManager& em, double deltaTime);
    
    // Helper functions
    float CalculateAtmosphericDrag(const AtmosphereComponent& atmo, float velocity, float altitude);
    float CalculateHeatLoad(float velocity, float density);
    bool IsLandingSafe(const LandingGearComponent& gear, const glm::vec3& velocity, const glm::vec3& surfaceNormal);
};

class EVASystem : public System {
public:
    EVASystem() : System(SystemType::EVA) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void UpdateOxygenConsumption(EntityManager& em, double deltaTime);
    void UpdateSuitIntegrity(EntityManager& em, double deltaTime);
    void UpdateJetpack(EntityManager& em, double deltaTime);
    void CheckEnvironmentalDamage(EntityManager& em, double deltaTime);
};

class SurfaceVehicleSystem : public System {
public:
    SurfaceVehicleSystem() : System(SystemType::SurfaceVehicle) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void UpdateVehiclePhysics(EntityManager& em, double deltaTime);
    void UpdateFuelConsumption(EntityManager& em, double deltaTime);
    void HandleTerrainInteraction(EntityManager& em, double deltaTime);
};

class WeatherSystem : public System {
public:
    WeatherSystem() : System(SystemType::Weather) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void UpdateWeatherPatterns(EntityManager& em, double deltaTime);
    void ApplyWeatherEffects(EntityManager& em, double deltaTime);
    void GenerateWeatherEvents(EntityManager& em, double deltaTime);
    
    float weatherTransitionTimer = 0.0f;
    const float minTransitionTime = 600.0f; // 10 minutes
};

class DayNightCycleSystem : public System {
public:
    DayNightCycleSystem() : System(SystemType::DayNightCycle) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void UpdateSunPosition(EntityManager& em, double deltaTime);
    void UpdateAmbientLighting(EntityManager& em, double deltaTime);
};

class ResourceScanningSystem : public System {
public:
    ResourceScanningSystem() : System(SystemType::ResourceScanning) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void PerformScans(EntityManager& em, double deltaTime);
    void DetectResources(EntityManager& em, double deltaTime);
    void UpdateScanProgress(EntityManager& em, double deltaTime);
};

class MiningSystem : public System {
public:
    MiningSystem() : System(SystemType::Mining) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void ProcessMining(EntityManager& em, double deltaTime);
    void UpdateMiningEquipment(EntityManager& em, double deltaTime);
    void HandleHeatBuildup(EntityManager& em, double deltaTime);
    void DepositResources(EntityManager& em, int entityId, float amount);
};

class EnvironmentalHazardSystem : public System {
public:
    EnvironmentalHazardSystem() : System(SystemType::EnvironmentalHazard) {}
    
    void Update(EntityManager& em, double deltaTime) override;
    
private:
    void ApplyHazardDamage(EntityManager& em, double deltaTime);
    void CheckHazardProximity(EntityManager& em, double deltaTime);
    void UpdateHazardStates(EntityManager& em, double deltaTime);
};

} // namespace Nova
