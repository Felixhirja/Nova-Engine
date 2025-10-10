#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

// Event types for visual/audio feedback
enum class FeedbackEventType {
    // Shield events
    ShieldHit,
    ShieldDepleted,
    ShieldRecharging,
    ShieldFullyCharged,
    
    // Hull/damage events
    HullDamage,
    CriticalDamage,
    SubsystemFailure,
    HullBreach,
    
    // Weapon events
    WeaponFired,
    WeaponOverheat,
    AmmoEmpty,
    
    // Power/energy events
    PowerOverload,
    PowerCritical,
    EnergyDiverted,
    
    // System alerts
    WarningLowShields,
    WarningLowPower,
    WarningOverheating,
    AlarmCritical,
    AlarmEvacuate
};

// Severity levels for alerts
enum class AlertSeverity {
    Info,       // Blue - informational
    Warning,    // Yellow - attention needed
    Critical,   // Red - immediate action required
    Emergency   // Flashing red - life threatening
};

// Feedback event data
struct FeedbackEvent {
    FeedbackEventType type;
    int entityId;
    AlertSeverity severity;
    
    // Additional context data
    double magnitude;           // Damage amount, shield percentage, etc.
    double x, y, z;            // World position for spatial effects
    std::string componentId;   // Which component triggered the event
    std::string message;       // Optional HUD message
    
    FeedbackEvent(FeedbackEventType t, int entity, AlertSeverity sev = AlertSeverity::Info)
        : type(t), entityId(entity), severity(sev), 
          magnitude(0.0), x(0.0), y(0.0), z(0.0) {}
};

// Event listener interface
class IFeedbackListener {
public:
    virtual ~IFeedbackListener() = default;
    virtual void OnFeedbackEvent(const FeedbackEvent& event) = 0;
};

// Event manager for decoupled feedback system
class FeedbackEventManager {
public:
    static FeedbackEventManager& Get() {
        static FeedbackEventManager instance;
        return instance;
    }
    
    // Register a listener
    void Subscribe(std::shared_ptr<IFeedbackListener> listener) {
        listeners_.push_back(listener);
    }
    
    // Emit an event to all listeners
    void Emit(const FeedbackEvent& event) {
        for (auto& listener : listeners_) {
            if (listener) {
                listener->OnFeedbackEvent(event);
            }
        }
    }
    
    // Clear all listeners (for testing/cleanup)
    void Clear() {
        listeners_.clear();
    }

private:
    FeedbackEventManager() = default;
    std::vector<std::shared_ptr<IFeedbackListener>> listeners_;
};
