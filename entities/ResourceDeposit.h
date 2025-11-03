#pragma once

#include "../engine/EntityCommon.h"

class ResourceDeposit : public IActor {
public:
    void Initialize() override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            
            // Position
            Position pos(0, 0, 0);
            em->AddComponent<Position>(entity, pos);
            
            // Resource deposit
            Nova::ResourceDepositComponent deposit;
            deposit.type = Nova::ResourceDepositComponent::ResourceType::IronOre;
            deposit.quantity = 5000.0f;
            deposit.density = 0.8f;
            deposit.miningDifficulty = 0.3f;
            deposit.radius = 15.0f;
            em->AddComponent<Nova::ResourceDepositComponent>(entity, deposit);
            
            // Visual representation
            DrawComponent draw;
            draw.mode = RenderMode::Mesh3D;
            draw.color = {0.6f, 0.4f, 0.2f, 1.0f};
            draw.scale = {deposit.radius, 2.0f, deposit.radius};
            em->AddComponent<DrawComponent>(entity, draw);
        }
    }
    
    void Update(double deltaTime) override {
        if (auto* em = context_.GetEntityManager()) {
            auto entity = context_.GetEntity();
            auto* deposit = em->GetComponent<Nova::ResourceDepositComponent>(entity);
            
            // Update visual based on quantity
            if (deposit && deposit->quantity <= 0.0f) {
                // Depleted - could remove or mark as empty
                if (auto* draw = em->GetComponent<DrawComponent>(entity)) {
                    draw->color.a = 0.3f; // Fade out
                }
            }
        }
    }
    
    std::string GetName() const override {
        return "ResourceDeposit";
    }
};
