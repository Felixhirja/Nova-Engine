#pragma once

#include "../engine/EntityCommon.h"

/**
 * ProjectileActor: Physics-based projectile actor
 * Simplified implementation - loads configuration from JSON
 */
class ProjectileActor : public IActor {
public:
    enum class ProjectileType {
        Bullet,
        Missile,
        Laser,
        Plasma
    };

    ProjectileActor(ProjectileType type = ProjectileType::Bullet,
               uint32_t ownerEntity = 0,
               double damage = 10.0)
        : projectileType_(type), ownerEntity_(ownerEntity), damage_(damage) {}
    ~ProjectileActor() override = default;

    void Initialize() override;
    std::string GetName() const override;
    void Update(double dt) override;

    // Projectile-specific methods
    ProjectileType GetProjectileType() const { return projectileType_; }

    uint32_t GetOwnerEntity() const { return ownerEntity_; }
    void SetOwnerEntity(uint32_t entity) { ownerEntity_ = entity; }

    double GetDamage() const { return damage_; }
    void SetDamage(double damage) { damage_ = damage; }

    double GetLifetime() const { return lifetime_; }
    void SetLifetime(double lifetime) { lifetime_ = lifetime; }

    bool IsExpired() const { return currentLifetime_ >= lifetime_; }

private:
    ProjectileType projectileType_;
    uint32_t ownerEntity_;
    double damage_;
    double lifetime_ = 10.0; // seconds
    double currentLifetime_ = 0.0;

    std::unique_ptr<simplejson::JsonObject> config_;

    // Projectile-specific properties loaded from JSON
    std::string name_;
    double speed_ = 500.0;
    std::string model_;
    std::vector<std::string> effects_;
};

// Inline implementations

inline void ProjectileActor::Initialize() {
    // Load configuration from JSON
    config_ = ActorConfig::LoadFromFile("assets/actors/projectile.json");
    if (config_) {
        name_ = ActorConfig::GetString(*config_, "name", "Plasma Bolt");
        speed_ = ActorConfig::GetNumber(*config_, "speed", 500.0);
        damage_ = ActorConfig::GetNumber(*config_, "damage", 150.0);
        lifetime_ = ActorConfig::GetNumber(*config_, "lifetime", 10.0);
        model_ = ActorConfig::GetString(*config_, "model", "plasma_bolt");
    }

    // Set up basic ECS components
    if (auto* em = context_.GetEntityManager()) {
        // Add position and velocity components
        em->AddComponent<Position>(context_.GetEntity(), std::make_shared<Position>(0.0, 0.0, 0.0));
        em->AddComponent<Velocity>(context_.GetEntity(), std::make_shared<Velocity>(0.0, 0.0, speed_));
        // Auto-add ViewportID component for rendering
        em->AddComponent<ViewportID>(context_.GetEntity(), std::make_shared<ViewportID>(0));
    }
}

inline std::string ProjectileActor::GetName() const {
    if (!name_.empty()) return name_;
    
    switch (projectileType_) {
        case ProjectileType::Bullet: return "Bullet";
        case ProjectileType::Missile: return "Missile";
        case ProjectileType::Laser: return "Laser";
        case ProjectileType::Plasma: return "Plasma";
        default: return "Projectile";
    }
}

inline void ProjectileActor::Update(double dt) {
    currentLifetime_ += dt;
    // TODO: Implement projectile behavior
}