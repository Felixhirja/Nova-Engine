#include "../src/FeedbackEvent.h"
#include "../src/VisualFeedbackSystem.h"
#include "../src/AudioFeedbackSystem.h"
#include "../src/HUDAlertSystem.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Testing feedback systems..." << std::endl;
    
    // Create feedback systems
    auto visualFeedback = std::make_shared<VisualFeedbackSystem>();
    auto audioFeedback = std::make_shared<AudioFeedbackSystem>();
    auto hudAlerts = std::make_shared<HUDAlertSystem>();
    
    // Initialize audio system
    if (!audioFeedback->Initialize()) {
        std::cerr << "Failed to initialize audio system" << std::endl;
        return 1;
    }
    
    // Register listeners with event manager
    FeedbackEventManager::Get().Subscribe(visualFeedback);
    FeedbackEventManager::Get().Subscribe(audioFeedback);
    FeedbackEventManager::Get().Subscribe(hudAlerts);
    
    std::cout << "Feedback systems initialized." << std::endl;
    
    // Test 1: Shield hit event
    {
        FeedbackEvent event(FeedbackEventType::ShieldHit, 1, AlertSeverity::Info);
        event.magnitude = 50.0;
        event.x = 10.0;
        event.y = 5.0;
        event.z = 0.0;
        FeedbackEventManager::Get().Emit(event);
        
        visualFeedback->Update(0.016);  // One frame
        hudAlerts->Update(0.016);
        
        if (visualFeedback->GetParticles().empty()) {
            std::cerr << "Shield hit should create particles" << std::endl;
            return 2;
        }
        
        if (hudAlerts->GetActiveAlerts().empty()) {
            std::cerr << "Shield hit should create HUD alert" << std::endl;
            return 3;
        }
        
        std::cout << "✓ Shield hit event test passed" << std::endl;
    }
    
    // Test 2: Shield depleted (critical event)
    {
        FeedbackEvent event(FeedbackEventType::ShieldDepleted, 1, AlertSeverity::Critical);
        FeedbackEventManager::Get().Emit(event);
        
        hudAlerts->Update(0.016);
        
        const auto& alerts = hudAlerts->GetActiveAlerts();
        bool foundCritical = false;
        for (const auto& alert : alerts) {
            if (alert.severity == AlertSeverity::Critical && alert.flashing) {
                foundCritical = true;
                break;
            }
        }
        
        if (!foundCritical) {
            std::cerr << "Shield depleted should create flashing critical alert" << std::endl;
            return 4;
        }
        
        std::cout << "✓ Shield depleted event test passed" << std::endl;
    }
    
    // Test 3: Alert priority sorting
    {
        hudAlerts->ClearAll();
        
        // Post alerts in reverse priority order
        hudAlerts->PostAlert("Info message", AlertSeverity::Info, 5.0);
        hudAlerts->PostAlert("Emergency message", AlertSeverity::Emergency, 5.0);
        hudAlerts->PostAlert("Warning message", AlertSeverity::Warning, 5.0);
        hudAlerts->PostAlert("Critical message", AlertSeverity::Critical, 5.0);
        
        hudAlerts->Update(0.016);
        
        const auto& alerts = hudAlerts->GetActiveAlerts();
        if (alerts.size() != 4) {
            std::cerr << "Should have 4 alerts, got " << alerts.size() << std::endl;
            return 5;
        }
        
        // Check priority order (Emergency > Critical > Warning > Info)
        if (alerts[0].severity != AlertSeverity::Emergency ||
            alerts[1].severity != AlertSeverity::Critical ||
            alerts[2].severity != AlertSeverity::Warning ||
            alerts[3].severity != AlertSeverity::Info) {
            std::cerr << "Alerts not sorted by priority correctly" << std::endl;
            return 6;
        }
        
        std::cout << "✓ Alert priority sorting test passed" << std::endl;
    }
    
    // Test 4: Alert expiration
    {
        hudAlerts->ClearAll();
        hudAlerts->PostAlert("Expiring message", AlertSeverity::Info, 0.5);
        
        hudAlerts->Update(0.016);
        if (hudAlerts->GetActiveAlerts().empty()) {
            std::cerr << "Alert should still be active after one frame" << std::endl;
            return 7;
        }
        
        hudAlerts->Update(0.6);  // Expire the alert
        if (!hudAlerts->GetActiveAlerts().empty()) {
            std::cerr << "Alert should have expired" << std::endl;
            return 8;
        }
        
        std::cout << "✓ Alert expiration test passed" << std::endl;
    }
    
    // Test 5: Max visible alerts limit
    {
        hudAlerts->ClearAll();
        hudAlerts->SetMaxVisibleAlerts(3);
        
        // Post 5 alerts
        for (int i = 0; i < 5; ++i) {
            hudAlerts->PostAlert("Alert " + std::to_string(i), AlertSeverity::Info, 5.0);
        }
        
        hudAlerts->Update(0.016);
        
        if (hudAlerts->GetActiveAlerts().size() != 3) {
            std::cerr << "Should only show 3 alerts max, got " 
                      << hudAlerts->GetActiveAlerts().size() << std::endl;
            return 9;
        }
        
        std::cout << "✓ Max visible alerts test passed" << std::endl;
    }
    
    // Test 6: Particle lifetime and physics
    {
        FeedbackEvent event(FeedbackEventType::CriticalDamage, 1, AlertSeverity::Critical);
        event.magnitude = 100.0;
        event.x = 0.0;
        event.y = 0.0;
        event.z = 10.0;
        FeedbackEventManager::Get().Emit(event);
        
        size_t initialParticleCount = visualFeedback->GetParticles().size();
        
        // Update for 2 seconds
        for (int i = 0; i < 120; ++i) {
            visualFeedback->Update(0.016);
        }
        
        size_t finalParticleCount = visualFeedback->GetParticles().size();
        
        if (finalParticleCount >= initialParticleCount) {
            std::cerr << "Particles should decay over time" << std::endl;
            return 10;
        }
        
        std::cout << "✓ Particle lifetime test passed" << std::endl;
    }
    
    // Test 7: Screen shake decay
    {
        FeedbackEvent event(FeedbackEventType::HullDamage, 1);
        event.magnitude = 100.0;
        FeedbackEventManager::Get().Emit(event);
        
        visualFeedback->Update(0.016);
        
        double initialShake = visualFeedback->GetScreenShake();
        if (initialShake <= 0.0) {
            std::cerr << "Screen shake should be triggered by hull damage" << std::endl;
            return 11;
        }
        
        // Update for 1 second
        for (int i = 0; i < 60; ++i) {
            visualFeedback->Update(0.016);
        }
        
        double finalShake = visualFeedback->GetScreenShake();
        if (finalShake >= initialShake) {
            std::cerr << "Screen shake should decay over time" << std::endl;
            return 12;
        }
        
        std::cout << "✓ Screen shake decay test passed" << std::endl;
    }
    
    // Test 8: Duplicate alert suppression
    {
        hudAlerts->ClearAll();
        
        hudAlerts->PostAlert("Duplicate test", AlertSeverity::Warning, 5.0);
        hudAlerts->PostAlert("Duplicate test", AlertSeverity::Warning, 5.0);
        hudAlerts->PostAlert("Duplicate test", AlertSeverity::Warning, 5.0);
        
        hudAlerts->Update(0.016);
        
        if (hudAlerts->GetActiveAlerts().size() != 1) {
            std::cerr << "Should suppress duplicate alerts, got " 
                      << hudAlerts->GetActiveAlerts().size() << std::endl;
            return 13;
        }
        
        std::cout << "✓ Duplicate alert suppression test passed" << std::endl;
    }
    
    // Cleanup
    audioFeedback->Shutdown();
    FeedbackEventManager::Get().Clear();
    
    std::cout << "\nAll feedback system tests passed!" << std::endl;
    return 0;
}
