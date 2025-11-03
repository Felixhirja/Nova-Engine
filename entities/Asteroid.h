#pragma once

#include "../engine/EntityCommon.h"
#include "../engine/ecs/MiningComponents.h"
#include <random>

class Asteroid : public IActor {
private:
    Nova::ResourceType primaryResource_;
    float quantity_;
    bool hasHazard_;
    
public:
    Asteroid(Nova::ResourceType resource = Nova::ResourceType::IronOre,
             float quantity = 5000.0f, bool hasHazard = false)
        : primaryResource_(resource), quantity_(quantity), hasHazard_(hasHazard) {}
    
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Position
            em->AddComponent<Position>(entity, {0, 0, 0});
            
            // Velocity (slowly rotating/drifting)
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> velDist(-2.0f, 2.0f);
            em->AddComponent<Velocity>(entity, {
                velDist(gen), velDist(gen), velDist(gen)
            });
            
            // Enhanced resource deposit
            Nova::EnhancedResourceDepositComponent deposit;
            deposit.primaryResource = primaryResource_;
            deposit.primaryQuantity = quantity_;
            
            // Random secondary resource
            std::uniform_int_distribution<int> typeDist(0, 10);
            deposit.secondaryResource = static_cast<Nova::ResourceType>(typeDist(gen));
            deposit.secondaryQuantity = quantity_ * 0.1f; // 10% secondary
            
            // Random properties
            std::uniform_real_distribution<float> floatDist(0.0f, 1.0f);
            deposit.density = 0.3f + floatDist(gen) * 0.7f;
            deposit.miningDifficulty = floatDist(gen);
            deposit.hardness = floatDist(gen);
            deposit.temperature = 100.0f + floatDist(gen) * 200.0f;
            deposit.radiation = floatDist(gen) * 0.5f;
            
            // Size based on quantity
            deposit.radius = std::sqrt(quantity_ / 100.0f);
            
            // Rotation
            deposit.rotationSpeed = floatDist(gen) * 0.1f;
            
            // Instability chance
            if (floatDist(gen) < 0.1f) {
                deposit.unstable = true;
                deposit.instabilityTimer = 60.0f + floatDist(gen) * 120.0f;
            }
            
            deposit.discovered = false;
            deposit.surveyed = false;
            
            em->AddComponent<Nova::EnhancedResourceDepositComponent>(entity, deposit);
            
            // Visual representation
            DrawComponent draw;
            draw.mode = DrawComponent::RenderMode::Mesh3D;
            
            // Color based on primary resource
            switch (primaryResource_) {
                case Nova::ResourceType::IronOre:
                    draw.SetTint(0.6f, 0.4f, 0.3f); // Rusty brown
                    break;
                case Nova::ResourceType::CopperOre:
                    draw.SetTint(0.7f, 0.5f, 0.3f); // Copper
                    break;
                case Nova::ResourceType::TitaniumOre:
                    draw.SetTint(0.7f, 0.7f, 0.8f); // Silver-gray
                    break;
                case Nova::ResourceType::PlatinumOre:
                case Nova::ResourceType::GoldOre:
                    draw.SetTint(0.9f, 0.8f, 0.3f); // Golden
                    break;
                case Nova::ResourceType::ExoticCrystals:
                    draw.SetTint(0.6f, 0.3f, 0.9f); // Purple
                    break;
                case Nova::ResourceType::WaterIce:
                    draw.SetTint(0.8f, 0.9f, 1.0f); // Ice blue
                    break;
                default:
                    draw.SetTint(0.5f, 0.5f, 0.5f); // Gray
                    break;
            }
            
            draw.SetScale(deposit.radius);
            em->AddComponent<DrawComponent>(entity, draw);
            
            // Add hazard if specified
            if (hasHazard_) {
                Nova::MiningHazardComponent hazard;
                
                // Random hazard type
                std::uniform_int_distribution<int> hazardDist(0, 6);
                hazard.type = static_cast<Nova::MiningHazardComponent::HazardType>(hazardDist(gen));
                
                hazard.intensity = 0.3f + floatDist(gen) * 0.5f;
                hazard.damageRate = hazard.intensity * 2.0f;
                hazard.radius = deposit.radius * 2.0f;
                hazard.active = true;
                
                // Some hazards are intermittent
                if (floatDist(gen) < 0.4f) {
                    hazard.intermittent = true;
                    hazard.cycleTime = 20.0f + floatDist(gen) * 40.0f;
                }
                
                hazard.warningRange = hazard.radius * 2.5f;
                hazard.detected = false;
                
                em->AddComponent<Nova::MiningHazardComponent>(entity, hazard);
            }
            
            // Health (asteroids can be destroyed)
            Health health;
            health.max = quantity_ / 50.0f; // More resources = more durable
            health.current = health.max;
            em->AddComponent<Health>(entity, health);
        }
    }
    
    void Update(double deltaTime) override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            auto* deposit = em->GetComponent<Nova::EnhancedResourceDepositComponent>(entity);
            auto* draw = em->GetComponent<DrawComponent>(entity);
            
            if (deposit && draw) {
                // Update visual based on remaining resources
                float resourcePercent = deposit->primaryQuantity / quantity_;
                
                // Fade and shrink as depleted
                draw->opacity = 0.3f + resourcePercent * 0.7f;
                float currentRadius = deposit->radius * std::sqrt(resourcePercent);
                draw->SetScale(currentRadius);
                
                // Handle instability
                if (deposit->unstable) {
                    deposit->instabilityTimer -= static_cast<float>(deltaTime);
                    
                    // Pulsing effect for unstable asteroids
                    float pulse = static_cast<float>(std::sin(context_.GetTime() * 3.0));
                    draw->tintR = std::min(1.0f, draw->tintR + pulse * 0.2f);
                    
                    if (deposit->instabilityTimer <= 0.0f) {
                        // Asteroid fragments/explodes
                        if (auto* health = em->GetComponent<Health>(entity)) {
                            health->current = 0.0f;
                        }
                    }
                }
                
                // Show if discovered
                if (deposit->discovered && !deposit->surveyed) {
                    // Slight glow for discovered but not surveyed
                    float glow = static_cast<float>(std::sin(context_.GetTime() * 2.0)) * 0.1f;
                    draw->tintR += glow;
                    draw->tintG += glow;
                    draw->tintB += glow;
                }
            }
            
            // Update hazard detection range
            if (auto* hazard = em->GetComponent<Nova::MiningHazardComponent>(entity)) {
                if (!hazard->detected && deposit && deposit->discovered) {
                    hazard->detected = true;
                }
            }
        }
    }
    
    std::string GetName() const override {
        return "Asteroid";
    }
};
