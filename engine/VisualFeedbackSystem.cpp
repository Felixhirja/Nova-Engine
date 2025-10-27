#include "VisualFeedbackSystem.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

VisualFeedbackSystem::VisualFeedbackSystem() {
    particles_.reserve(1000);  // Pre-allocate for performance
}

void VisualFeedbackSystem::Update(double dt) {
    // Update particles
    for (auto it = particles_.begin(); it != particles_.end(); ) {
        it->lifetime -= dt;
        if (it->lifetime <= 0.0) {
            it = particles_.erase(it);
        } else {
            // Update position
            it->x += it->vx * dt;
            it->y += it->vy * dt;
            it->z += it->vz * dt;
            
            // Apply gravity
            it->vz -= 9.8 * dt;
            
            // Fade out
            it->a = static_cast<float>(it->lifetime / it->maxLifetime);
            
            ++it;
        }
    }
    
    // Decay screen shake
    if (screenShakeIntensity_ > 0.0) {
        screenShakeIntensity_ = std::max(0.0, screenShakeIntensity_ - screenShakeDecay_ * dt);
    }
}

void VisualFeedbackSystem::Render() {
    // TODO: Implement actual rendering using OpenGL
    // This is a placeholder for the rendering logic
    // Will be integrated with Viewport3D for actual particle rendering
}

void VisualFeedbackSystem::OnFeedbackEvent(const FeedbackEvent& event) {
    switch (event.type) {
        case FeedbackEventType::ShieldHit:
            SpawnShieldImpact(event.x, event.y, event.z, event.magnitude);
            TriggerScreenShake(event.magnitude * 0.01);
            break;
            
        case FeedbackEventType::ShieldDepleted:
            SpawnExplosion(event.x, event.y, event.z, 50.0);
            TriggerScreenShake(0.3);
            break;
            
        case FeedbackEventType::HullDamage:
            SpawnSparks(event.x, event.y, event.z, 20, 1.0f, 0.8f, 0.3f);
            TriggerScreenShake(event.magnitude * 0.02);
            break;
            
        case FeedbackEventType::CriticalDamage:
            SpawnExplosion(event.x, event.y, event.z, event.magnitude);
            SpawnSparks(event.x, event.y, event.z, 50, 1.0f, 0.3f, 0.0f);
            TriggerScreenShake(0.5);
            break;
            
        case FeedbackEventType::WeaponFired:
            // Muzzle flash effect
            SpawnSparks(event.x, event.y, event.z, 5, 1.0f, 1.0f, 0.5f);
            break;
            
        case FeedbackEventType::SubsystemFailure:
            SpawnSparks(event.x, event.y, event.z, 30, 1.0f, 0.0f, 0.0f);
            TriggerScreenShake(0.2);
            break;
            
        default:
            break;
    }
}

void VisualFeedbackSystem::SpawnSparks(double x, double y, double z, int count, float r, float g, float b) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        p.z = z;
        
        // Random velocity
        float angle = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
        float speed = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.vz = 2.0f + static_cast<float>(rand()) / RAND_MAX * 3.0f;
        
        p.lifetime = 0.5 + static_cast<double>(rand()) / RAND_MAX * 0.5;
        p.maxLifetime = p.lifetime;
        p.r = r;
        p.g = g;
        p.b = b;
        p.a = 1.0f;
        p.size = 0.1f + static_cast<float>(rand()) / RAND_MAX * 0.2f;
        
        particles_.push_back(p);
    }
}

void VisualFeedbackSystem::SpawnShieldImpact(double x, double y, double z, double magnitude) {
    // Blue/cyan shield hit effect
    int count = static_cast<int>(magnitude * 0.5) + 10;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        p.z = z;
        
        // Radial burst
        float angle = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
        float speed = 1.0f + static_cast<float>(rand()) / RAND_MAX * 2.0f;
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.vz = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 2.0f;
        
        p.lifetime = 0.3 + static_cast<double>(rand()) / RAND_MAX * 0.3;
        p.maxLifetime = p.lifetime;
        p.r = 0.3f;
        p.g = 0.7f;
        p.b = 1.0f;
        p.a = 1.0f;
        p.size = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f;
        
        particles_.push_back(p);
    }
}

void VisualFeedbackSystem::SpawnExplosion(double x, double y, double z, double magnitude) {
    // Orange/red explosion effect
    int count = static_cast<int>(magnitude) + 30;
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.x = x;
        p.y = y;
        p.z = z;
        
        // Spherical burst
        float angle1 = static_cast<float>(rand()) / RAND_MAX * 6.28318f;
        float angle2 = static_cast<float>(rand()) / RAND_MAX * 3.14159f;
        float speed = 3.0f + static_cast<float>(rand()) / RAND_MAX * 4.0f;
        p.vx = std::cos(angle1) * std::sin(angle2) * speed;
        p.vy = std::sin(angle1) * std::sin(angle2) * speed;
        p.vz = std::cos(angle2) * speed;
        
        p.lifetime = 0.8 + static_cast<double>(rand()) / RAND_MAX * 0.7;
        p.maxLifetime = p.lifetime;
        
        // Color gradient from yellow to red
        float t = static_cast<float>(rand()) / RAND_MAX;
        p.r = 1.0f;
        p.g = 0.5f + t * 0.5f;
        p.b = 0.0f;
        p.a = 1.0f;
        p.size = 0.3f + static_cast<float>(rand()) / RAND_MAX * 0.5f;
        
        particles_.push_back(p);
    }
}

void VisualFeedbackSystem::TriggerScreenShake(double intensity) {
    screenShakeIntensity_ = std::min(1.0, screenShakeIntensity_ + intensity);
}
