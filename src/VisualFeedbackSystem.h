#pragma once

#include "FeedbackEvent.h"
#include <vector>
#include <memory>

// Particle for visual effects
struct Particle {
    double x, y, z;
    double vx, vy, vz;
    double lifetime;
    double maxLifetime;
    float r, g, b, a;
    float size;
    
    bool IsAlive() const { return lifetime > 0.0; }
};

// Visual feedback system handles particles, decals, and screen effects
class VisualFeedbackSystem : public IFeedbackListener {
public:
    VisualFeedbackSystem();
    
    void Update(double dt);
    void Render();
    
    // IFeedbackListener implementation
    void OnFeedbackEvent(const FeedbackEvent& event) override;
    
    // Access particles for rendering
    const std::vector<Particle>& GetParticles() const { return particles_; }
    
    // Screen shake intensity (0.0 = none, 1.0 = max)
    double GetScreenShake() const { return screenShakeIntensity_; }
    
private:
    std::vector<Particle> particles_;
    double screenShakeIntensity_ = 0.0;
    double screenShakeDecay_ = 5.0;  // Decay per second
    
    // Effect generators
    void SpawnSparks(double x, double y, double z, int count, float r, float g, float b);
    void SpawnShieldImpact(double x, double y, double z, double magnitude);
    void SpawnExplosion(double x, double y, double z, double magnitude);
    void TriggerScreenShake(double intensity);
};
