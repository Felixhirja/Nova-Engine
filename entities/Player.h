#pragma once

#include "../engine/EntityCommon.h"
#include "CameraViewState.h"
#include <iostream>

/**
 * Player: Player-controlled actor - high level interface only
 */
class Player : public IActor {
public:
    Player() {
        std::cout << "[Player] Constructor called" << std::endl;
    }
    virtual ~Player() {
        std::cout << "[Player] Destructor called" << std::endl;
    }

    // IActor interface
    void Initialize() override {
        std::cout << "[Player] Initialize() called - Entity ID: " << context_.GetEntity() << std::endl;
        
        // Add ViewportID component for automatic rendering
        if (auto* em = context_.GetEntityManager()) {
            std::cout << "[Player] EntityManager available, adding components..." << std::endl;
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(context_.GetEntity(), vp);
            std::cout << "[Player] Added ViewportID component" << std::endl;
            
            // Add DrawComponent for visual rendering
            auto draw = std::make_shared<DrawComponent>();
            draw->mode = DrawComponent::RenderMode::Mesh3D;
            draw->visible = true;
            draw->meshHandle = 0;  // 0 = use fallback cube
            draw->meshScale = 0.5f;
            draw->SetTint(0.2f, 0.8f, 1.0f);  // Cyan color for player
            em->AddComponent<DrawComponent>(context_.GetEntity(), draw);
            std::cout << "[Player] Added DrawComponent (Mesh3D, cyan tint)" << std::endl;
            
            // Add CameraComponent so camera follows this entity
            auto cam = std::make_shared<CameraComponent>();
            cam->isActive = true;
            cam->priority = 100;  // High priority for player
            em->AddComponent<CameraComponent>(context_.GetEntity(), cam);
            std::cout << "[Player] Added CameraComponent (priority 100)" << std::endl;
            
            // Add movement components
            auto velocity = std::make_shared<Velocity>();
            velocity->vx = 0.0;
            velocity->vy = 0.0;
            velocity->vz = 0.0;
            em->AddComponent<Velocity>(context_.GetEntity(), velocity);
            std::cout << "[Player] Added Velocity component" << std::endl;
            
            auto playerPhysics = std::make_shared<PlayerPhysics>();
            playerPhysics->enableGravity = true;
            playerPhysics->thrustMode = false;
            playerPhysics->isGrounded = true;
            playerPhysics->gravity = -9.8;
            playerPhysics->jumpImpulse = 6.0;
            playerPhysics->maxAscentSpeed = 10.0;
            playerPhysics->maxDescentSpeed = -20.0;
            playerPhysics->thrustAcceleration = 8.0;
            playerPhysics->thrustDamping = 6.0;
            em->AddComponent<PlayerPhysics>(context_.GetEntity(), playerPhysics);
            std::cout << "[Player] Added PlayerPhysics component (gravity enabled)" << std::endl;
            
            std::cout << "[Player] Initialize() complete - all components added" << std::endl;
        } else {
            std::cerr << "[Player] ERROR: EntityManager not available!" << std::endl;
        }
    }
    void Update(double deltaTime) override { 
        static int updateCount = 0;
        if (++updateCount % 120 == 0) {  // Log every 2 seconds at 60fps
            std::cout << "[Player] Update() called - deltaTime: " << deltaTime << std::endl;
        }
        (void)deltaTime; 
    }
    std::string GetName() const override { return "Player"; }

    // High-level interface for MainLoop
    CameraViewState GetCameraViewState() const { return cameraState_; }
    void SetCameraViewState(const CameraViewState& state) { cameraState_ = state; }
    
private:
    CameraViewState cameraState_;
};
