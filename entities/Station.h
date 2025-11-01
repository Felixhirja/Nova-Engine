#pragma once

#include "../engine/EntityCommon.h"

/**
 * Station: Space station actor for docking and hub interactions
 * Simplified implementation - loads configuration from JSON
 */
class Station : public IActor {
public:
    enum class StationType {
        Trading,
        Military,
        Mining,
        Research
    };

    Station(StationType type = StationType::Trading) : stationType_(type) {}
    ~Station() override = default;

    void Initialize() override;
    std::string GetName() const override;
    void Update(double dt) override;

    // Station-specific methods
    StationType GetStationType() const { return stationType_; }

    void SetFaction(const std::string& faction) { faction_ = faction; }
    const std::string& GetFaction() const { return faction_; }

    void SetDockingEnabled(bool enabled) { dockingEnabled_ = enabled; }
    bool IsDockingEnabled() const { return dockingEnabled_; }

    // Docking management
    bool RequestDocking(uint32_t shipEntity);
    void Undock(uint32_t shipEntity);

private:
    StationType stationType_;
    std::string faction_ = "neutral";
    bool dockingEnabled_ = true;
    std::vector<uint32_t> dockedShips_;

    std::unique_ptr<simplejson::JsonObject> config_;

    // Station-specific properties loaded from JSON
    std::string name_;
    double health_ = 5000.0;
    double shield_ = 2000.0;
    std::string model_;
    int dockingCapacity_ = 4;
    std::vector<std::string> services_;
    std::string behaviorScript_;
};

// Inline implementations

inline void Station::Initialize() {
    // Load configuration from JSON
    config_ = ActorConfig::LoadFromFile("assets/actors/station.json");
    if (config_) {
        name_ = ActorConfig::GetString(*config_, "name", "Space Station");
        health_ = ActorConfig::GetNumber(*config_, "health", 5000.0);
        shield_ = ActorConfig::GetNumber(*config_, "shield", 2000.0);
        model_ = ActorConfig::GetString(*config_, "model", "station_01");
        dockingCapacity_ = static_cast<int>(ActorConfig::GetNumber(*config_, "dockingCapacity", 4.0));
        faction_ = ActorConfig::GetString(*config_, "faction", "neutral");
        behaviorScript_ = ActorConfig::GetString(*config_, "behaviorScript", "");
    }

    // Set up basic ECS components
    if (auto* em = context_.GetEntityManager()) {
        // Add position component
        em->AddComponent<Position>(context_.GetEntity(), std::make_shared<Position>(0.0, 0.0, 0.0));
        // Add velocity (stations are stationary)
        em->AddComponent<Velocity>(context_.GetEntity(), std::make_shared<Velocity>(0.0, 0.0, 0.0));
        // Auto-add ViewportID component for rendering
        em->AddComponent<ViewportID>(context_.GetEntity(), std::make_shared<ViewportID>(0));
    }
}

inline std::string Station::GetName() const {
    return name_.empty() ? "Unnamed Station" : name_;
}

inline void Station::Update(double dt) {
    // TODO: Update station systems
    // - Process docked ships
    // - Run behavior scripts
    // - Update services
    (void)dt;
}

inline bool Station::RequestDocking(uint32_t shipEntity) {
    if (!dockingEnabled_) return false;
    if (static_cast<int>(dockedShips_.size()) >= dockingCapacity_) return false;

    // Check if already docked
    auto it = std::find(dockedShips_.begin(), dockedShips_.end(), shipEntity);
    if (it != dockedShips_.end()) return true;

    dockedShips_.push_back(shipEntity);
    return true;
}

inline void Station::Undock(uint32_t shipEntity) {
    auto it = std::find(dockedShips_.begin(), dockedShips_.end(), shipEntity);
    if (it != dockedShips_.end()) {
        dockedShips_.erase(it);
    }
}