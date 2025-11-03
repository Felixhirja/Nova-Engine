#pragma once

#include <string>
#include <memory>

// Forward declarations
struct Position;
struct Velocity;
struct PlayerPhysics;
struct DrawComponent;
struct CameraComponent;
struct PlayerController;

/**
 * PlayerConfig - Designer-friendly player configuration
 * Load settings from JSON files without touching core code
 */
struct PlayerConfig {
    // Spawn settings
    struct {
        double x = 0.0, y = 0.0, z = 0.0;
    } spawnPosition;
    
    // Movement settings
    struct {
        double forwardSpeed = 5.0;
        double backwardSpeed = 5.0;
        double strafeSpeed = 5.0;
        double upDownSpeed = 5.0;
        double acceleration = 4.0;
        double deceleration = 4.0;
        double friction = 0.0;
    } movement;
    
    // Physics settings
    struct {
        bool enableGravity = false;
        double gravityStrength = -9.8;
        double maxAscentSpeed = 10.0;
        double maxDescentSpeed = -20.0;
    } physics;
    
    // Visual settings
    struct {
        float r = 0.2f, g = 0.8f, b = 1.0f;  // RGB color
        float scale = 0.5f;
        int meshId = 0;  // 0 = default cube
    } visual;
    
    // Camera settings
    struct {
        int priority = 100;
        bool isActive = true;
    } camera;
    
    // Load from JSON file
    static PlayerConfig LoadFromFile(const std::string& filePath);
    
    // Apply configuration to components
    void ApplyToPosition(std::shared_ptr<Position> pos) const;
    void ApplyToPlayerPhysics(std::shared_ptr<PlayerPhysics> physics) const;
    void ApplyToDrawComponent(std::shared_ptr<DrawComponent> draw) const;
    void ApplyToCameraComponent(std::shared_ptr<CameraComponent> cam) const;
    
    // Get default configuration
    static PlayerConfig GetDefault();
};