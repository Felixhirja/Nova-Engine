#pragma once

#include "FeedbackEvent.h"
#include <string>
#include <vector>
#include <queue>

// HUD alert message
struct HUDAlert {
    FeedbackEventType type;
    AlertSeverity severity;
    std::string message;
    double displayTime;       // How long to show (seconds)
    double timeRemaining;     // Time left on screen
    bool isDismissed;
    int priority;             // Higher = more important
    
    // Visual properties
    float r, g, b, a;         // Color and alpha
    bool flashing;            // Should flash for attention
    
    HUDAlert() : type(FeedbackEventType::ShieldHit), severity(AlertSeverity::Info),
                 displayTime(3.0), timeRemaining(3.0), isDismissed(false), 
                 priority(0), r(1.0f), g(1.0f), b(1.0f), a(1.0f), flashing(false) {}
};

// Comparison for priority queue
struct HUDAlertCompare {
    bool operator()(const HUDAlert& a, const HUDAlert& b) const {
        // Higher priority first, then by time remaining
        if (a.priority != b.priority) {
            return a.priority < b.priority;  // Lower value = lower priority
        }
        return a.timeRemaining < b.timeRemaining;
    }
};

// HUD alert system manages on-screen warning messages
class HUDAlertSystem : public IFeedbackListener {
public:
    HUDAlertSystem();
    
    void Update(double dt);
    
    // Get active alerts for rendering
    const std::vector<HUDAlert>& GetActiveAlerts() const { return activeAlerts_; }
    
    // Manually post an alert
    void PostAlert(const std::string& message, AlertSeverity severity, double displayTime = 3.0);
    
    // Clear all alerts
    void ClearAll();
    
    // Dismiss specific alert types
    void DismissAlertType(FeedbackEventType type);
    
    // Configuration
    void SetMaxVisibleAlerts(int max) { maxVisibleAlerts_ = max; }
    void SetEnableFlashing(bool enable) { enableFlashing_ = enable; }
    
    // IFeedbackListener implementation
    void OnFeedbackEvent(const FeedbackEvent& event) override;
    
private:
    std::vector<HUDAlert> activeAlerts_;
    int maxVisibleAlerts_ = 5;
    bool enableFlashing_ = true;
    double flashTimer_ = 0.0;
    
    // Alert creation helpers
    HUDAlert CreateAlertFromEvent(const FeedbackEvent& event);
    void AddAlert(const HUDAlert& alert);
    std::string GetDefaultMessage(FeedbackEventType type, double magnitude);
    void GetColorForSeverity(AlertSeverity severity, float& r, float& g, float& b);
    int GetPriorityForSeverity(AlertSeverity severity);
};
