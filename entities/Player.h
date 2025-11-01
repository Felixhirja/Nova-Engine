#pragma once

#include "../engine/EntityCommon.h"
#include "CameraViewState.h"

/**
 * Player: Player-controlled actor - high level interface only
 */
class Player : public IActor {
public:
    Player() = default;
    virtual ~Player() = default;

    // IActor interface
    void Initialize() override {
        // Add ViewportID component for automatic rendering
        if (auto* em = context_.GetEntityManager()) {
            auto vp = std::make_shared<ViewportID>();
            vp->viewportId = 0;
            em->AddComponent<ViewportID>(context_.GetEntity(), vp);
            
            // Add CameraComponent so camera follows this entity
            auto cam = std::make_shared<CameraComponent>();
            cam->isActive = true;
            cam->priority = 100;  // High priority for player
            em->AddComponent<CameraComponent>(context_.GetEntity(), cam);
        }
    }
    void Update(double deltaTime) override { (void)deltaTime; }
    std::string GetName() const override { return "Player"; }

    // High-level interface for MainLoop
    CameraViewState GetCameraViewState() const { return cameraState_; }
    void SetCameraViewState(const CameraViewState& state) { cameraState_ = state; }
    
private:
    CameraViewState cameraState_;
};
