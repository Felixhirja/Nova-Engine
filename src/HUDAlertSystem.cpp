#include "HUDAlertSystem.h"
#include <algorithm>
#include <cmath>

HUDAlertSystem::HUDAlertSystem() {
    activeAlerts_.reserve(10);
}

void HUDAlertSystem::Update(double dt) {
    flashTimer_ += dt;
    
    // Update all alerts
    for (auto it = activeAlerts_.begin(); it != activeAlerts_.end(); ) {
        it->timeRemaining -= dt;
        
        // Update alpha fade out in last second
        if (it->timeRemaining < 1.0) {
            it->a = static_cast<float>(it->timeRemaining);
        }
        
        // Update flashing effect
        if (it->flashing && enableFlashing_) {
            float flashPhase = std::sin(flashTimer_ * 6.0);  // 6 rad/s = ~1 Hz flash
            it->a = 0.5f + 0.5f * flashPhase;
            
            // Override fade for critical alerts
            if (it->timeRemaining < 1.0 && it->severity == AlertSeverity::Emergency) {
                it->a = 0.7f + 0.3f * flashPhase;
            }
        }
        
        // Remove dismissed or expired alerts
        if (it->isDismissed || it->timeRemaining <= 0.0) {
            it = activeAlerts_.erase(it);
        } else {
            ++it;
        }
    }
    
    // Sort by priority (highest first)
    std::sort(activeAlerts_.begin(), activeAlerts_.end(), 
              [](const HUDAlert& a, const HUDAlert& b) {
                  return a.priority > b.priority;
              });
    
    // Trim to max visible
    if (static_cast<int>(activeAlerts_.size()) > maxVisibleAlerts_) {
        activeAlerts_.resize(maxVisibleAlerts_);
    }
}

void HUDAlertSystem::PostAlert(const std::string& message, AlertSeverity severity, double displayTime) {
    HUDAlert alert;
    alert.message = message;
    alert.severity = severity;
    alert.displayTime = displayTime;
    alert.timeRemaining = displayTime;
    alert.priority = GetPriorityForSeverity(severity);
    GetColorForSeverity(severity, alert.r, alert.g, alert.b);
    alert.flashing = (severity >= AlertSeverity::Critical);
    
    AddAlert(alert);
}

void HUDAlertSystem::ClearAll() {
    activeAlerts_.clear();
}

void HUDAlertSystem::DismissAlertType(FeedbackEventType type) {
    for (auto& alert : activeAlerts_) {
        if (alert.type == type) {
            alert.isDismissed = true;
        }
    }
}

void HUDAlertSystem::OnFeedbackEvent(const FeedbackEvent& event) {
    HUDAlert alert = CreateAlertFromEvent(event);
    AddAlert(alert);
}

HUDAlert HUDAlertSystem::CreateAlertFromEvent(const FeedbackEvent& event) {
    HUDAlert alert;
    alert.type = event.type;
    alert.severity = event.severity;
    alert.message = event.message.empty() ? 
                    GetDefaultMessage(event.type, event.magnitude) : 
                    event.message;
    alert.priority = GetPriorityForSeverity(event.severity);
    
    // Set display duration based on severity
    switch (event.severity) {
        case AlertSeverity::Info:
            alert.displayTime = 2.0;
            break;
        case AlertSeverity::Warning:
            alert.displayTime = 4.0;
            break;
        case AlertSeverity::Critical:
            alert.displayTime = 6.0;
            alert.flashing = true;
            break;
        case AlertSeverity::Emergency:
            alert.displayTime = 10.0;
            alert.flashing = true;
            break;
    }
    
    alert.timeRemaining = alert.displayTime;
    GetColorForSeverity(event.severity, alert.r, alert.g, alert.b);
    
    return alert;
}

void HUDAlertSystem::AddAlert(const HUDAlert& alert) {
    // Check for duplicate messages
    for (const auto& existing : activeAlerts_) {
        if (existing.message == alert.message && !existing.isDismissed) {
            // Already showing this alert, refresh its timer
            return;
        }
    }
    
    activeAlerts_.push_back(alert);
}

std::string HUDAlertSystem::GetDefaultMessage(FeedbackEventType type, double magnitude) {
    switch (type) {
        case FeedbackEventType::ShieldHit:
            return "Shield Impact";
            
        case FeedbackEventType::ShieldDepleted:
            return "SHIELDS DOWN";
            
        case FeedbackEventType::ShieldRecharging:
            return "Shield Recharging";
            
        case FeedbackEventType::ShieldFullyCharged:
            return "Shields Fully Charged";
            
        case FeedbackEventType::HullDamage:
            if (magnitude > 50.0) {
                return "SEVERE HULL DAMAGE";
            }
            return "Hull Damage";
            
        case FeedbackEventType::CriticalDamage:
            return "CRITICAL DAMAGE - STRUCTURAL FAILURE";
            
        case FeedbackEventType::SubsystemFailure:
            return "SUBSYSTEM FAILURE";
            
        case FeedbackEventType::HullBreach:
            return "HULL BREACH DETECTED";
            
        case FeedbackEventType::WeaponFired:
            return ""; // Don't show alert for weapon fire
            
        case FeedbackEventType::WeaponOverheat:
            return "Weapon Overheating";
            
        case FeedbackEventType::AmmoEmpty:
            return "Ammunition Depleted";
            
        case FeedbackEventType::PowerOverload:
            return "POWER OVERLOAD";
            
        case FeedbackEventType::PowerCritical:
            return "CRITICAL POWER LEVELS";
            
        case FeedbackEventType::EnergyDiverted:
            return "Power Diverted";
            
        case FeedbackEventType::WarningLowShields:
            return "WARNING: Low Shield Capacity";
            
        case FeedbackEventType::WarningLowPower:
            return "WARNING: Low Power";
            
        case FeedbackEventType::WarningOverheating:
            return "WARNING: Thermal Overload";
            
        case FeedbackEventType::AlarmCritical:
            return "!!! CRITICAL ALERT !!!";
            
        case FeedbackEventType::AlarmEvacuate:
            return "!!! EVACUATE IMMEDIATELY !!!";
            
        default:
            return "System Alert";
    }
}

void HUDAlertSystem::GetColorForSeverity(AlertSeverity severity, float& r, float& g, float& b) {
    switch (severity) {
        case AlertSeverity::Info:
            r = 0.3f; g = 0.7f; b = 1.0f;  // Cyan
            break;
        case AlertSeverity::Warning:
            r = 1.0f; g = 0.9f; b = 0.2f;  // Yellow
            break;
        case AlertSeverity::Critical:
            r = 1.0f; g = 0.3f; b = 0.0f;  // Orange-red
            break;
        case AlertSeverity::Emergency:
            r = 1.0f; g = 0.0f; b = 0.0f;  // Red
            break;
    }
}

int HUDAlertSystem::GetPriorityForSeverity(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::Info:       return 1;
        case AlertSeverity::Warning:    return 2;
        case AlertSeverity::Critical:   return 3;
        case AlertSeverity::Emergency:  return 4;
        default:                        return 0;
    }
}
