#pragma once

#include "../engine/EntityCommon.h"

/**
 * NPC: Base class for non-player character spaceships
 * Provides AI behavior framework and spaceship functionality
 */
class NPC : public IActor {
public:
    enum class NPCType {
        Trader,
        Pirate,
        Patrol
    };

    NPC(NPCType type = NPCType::Trader) : npcType_(type) {}
    virtual ~NPC() = default;

    // IActor interface
    void Initialize() override;
    std::string GetName() const override;
    void Update(double dt) override;

    // NPC-specific methods
    NPCType GetNPCType() const { return npcType_; }

    void SetFaction(const std::string& faction) { faction_ = faction; }
    const std::string& GetFaction() const { return faction_; }

    void SetAIEnabled(bool enabled) { aiEnabled_ = enabled; }
    bool IsAIEnabled() const { return aiEnabled_; }

    // Spaceship properties
    void SetSpaceshipClass(SpaceshipClassType shipClass) { spaceshipClass_ = shipClass; }
    SpaceshipClassType GetSpaceshipClass() const { return spaceshipClass_; }

protected:
    virtual void UpdateAI(double dt) = 0;

private:
    NPCType npcType_;
    std::string faction_ = "neutral";
    bool aiEnabled_ = true;
    SpaceshipClassType spaceshipClass_ = SpaceshipClassType::Fighter;

    std::unique_ptr<simplejson::JsonObject> config_;

    // NPC-specific properties loaded from JSON
    std::string name_;
    double speed_ = 100.0;
    double health_ = 1000.0;
    double shield_ = 500.0;
    std::string model_;
};

/**
 * TraderNPC: Merchant spaceship that trades goods between stations
 */
class TraderNPC : public NPC {
public:
    TraderNPC() : NPC(NPCType::Trader) {}
    ~TraderNPC() override = default;

    void Initialize() override;
    std::string GetName() const override { return "TraderNPC"; }

protected:
    void UpdateAI(double dt) override;

private:
    std::unique_ptr<simplejson::JsonObject> traderConfig_;
};

/**
 * PirateNPC: Hostile spaceship that attacks traders and players
 */
class PirateNPC : public NPC {
public:
    PirateNPC() : NPC(NPCType::Pirate) {}
    ~PirateNPC() override = default;

    void Initialize() override;
    std::string GetName() const override { return "PirateNPC"; }

protected:
    void UpdateAI(double dt) override;

private:
    std::unique_ptr<simplejson::JsonObject> pirateConfig_;
};

/**
 * PatrolNPC: Military spaceship that patrols areas and responds to threats
 */
class PatrolNPC : public NPC {
public:
    PatrolNPC() : NPC(NPCType::Patrol) {}
    ~PatrolNPC() override = default;

    void Initialize() override;
    std::string GetName() const override { return "PatrolNPC"; }

protected:
    void UpdateAI(double dt) override;

private:
    std::unique_ptr<simplejson::JsonObject> patrolConfig_;
};

// Inline implementations

inline void NPC::Initialize() {
    config_ = ActorConfig::LoadFromFile("assets/actors/npc.json");
    if (config_) {
        name_ = ActorConfig::GetString(*config_, "name", "NPC");
        speed_ = ActorConfig::GetNumber(*config_, "speed", 100.0);
        health_ = ActorConfig::GetNumber(*config_, "health", 1000.0);
        shield_ = ActorConfig::GetNumber(*config_, "shield", 500.0);
        model_ = ActorConfig::GetString(*config_, "model", "npc_ship");
        faction_ = ActorConfig::GetString(*config_, "faction", "neutral");
    }

    // Set up basic ECS components
    if (auto* em = context_.GetEntityManager()) {
        em->AddComponent<Position>(context_.GetEntity(), std::make_shared<Position>(0.0, 0.0, 0.0));
        em->AddComponent<Velocity>(context_.GetEntity(), std::make_shared<Velocity>(0.0, 0.0, 0.0));
        // Auto-add ViewportID component for rendering
        em->AddComponent<ViewportID>(context_.GetEntity(), std::make_shared<ViewportID>(0));
    }
}

inline std::string NPC::GetName() const {
    if (!name_.empty()) return name_;
    
    switch (npcType_) {
        case NPCType::Trader: return "Trader";
        case NPCType::Pirate: return "Pirate";
        case NPCType::Patrol: return "Patrol";
        default: return "NPC";
    }
}

inline void NPC::Update(double dt) {
    if (aiEnabled_) {
        UpdateAI(dt);
    }
}

// TraderNPC implementations

inline void TraderNPC::Initialize() {
    NPC::Initialize();
    traderConfig_ = ActorConfig::LoadFromFile("assets/actors/trader_npc.json");
    SetFaction("traders_guild");
}

inline void TraderNPC::UpdateAI(double dt) {
    // TODO: Implement trader AI
    // - Navigate to trading stations
    // - Avoid pirates
    // - Trade goods
    (void)dt;
}

// PirateNPC implementations

inline void PirateNPC::Initialize() {
    NPC::Initialize();
    pirateConfig_ = ActorConfig::LoadFromFile("assets/actors/pirate_npc.json");
    SetFaction("pirates");
}

inline void PirateNPC::UpdateAI(double dt) {
    // TODO: Implement pirate AI
    // - Hunt traders
    // - Attack players
    // - Flee from patrols
    (void)dt;
}

// PatrolNPC implementations

inline void PatrolNPC::Initialize() {
    NPC::Initialize();
    patrolConfig_ = ActorConfig::LoadFromFile("assets/actors/patrol_npc.json");
    SetFaction("military");
}

inline void PatrolNPC::UpdateAI(double dt) {
    // TODO: Implement patrol AI
    // - Follow patrol routes
    // - Respond to distress calls
    // - Engage pirates
    (void)dt;
}