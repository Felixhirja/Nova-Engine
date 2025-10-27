#pragma once

#include <algorithm>
#include <cmath>

// Forward-declare; implemented elsewhere.
class Camera;
namespace physics { class IPhysicsEngine; }

namespace CameraFollow {

// --- Small math helpers (header-only, no cost with ODR-use) ---
constexpr double kPI  = 3.14159265358979323846;
constexpr double kTAU = 2.0 * kPI;

constexpr double DegToRad(double d) noexcept { return d * (kPI / 180.0); }
constexpr double RadToDeg(double r) noexcept { return r * (180.0 / kPI); }

// Exponential smoothing factor from "natural frequency" (Hz) and dt (s).
inline double ExpAlpha(double hz, double dt) noexcept {
    const double w = std::max(0.0, hz);
    return 1.0 - std::exp(-w * std::max(0.0, dt));
}

// Smoothstep for soft transitions
constexpr double SmoothStep(double t) noexcept {
    return t * t * (3.0 - 2.0 * t);
}

struct CameraFollowConfig {
    // --- Orbital framing (meters) ---
    double orbitDistance = 12.0; // radial distance on XZ
    double orbitHeight   = 3.0;  // vertical offset above player

    // --- Safety / world ---
    double minDistanceFromPlayer = 2.0; // enforce min camera-to-player distance
    double groundLevel = 0.5;           // world Y of base plane
    double terrainBuffer = 1.0;         // min Y above groundLevel

    // --- Global smoothing (natural frequencies, in Hz) ---
    double transitionSpeed   = 3.0;  // lock/unlock blend
    double posResponsiveness = 10.0; // position smoothing
    double rotResponsiveness = 12.0; // rotation smoothing
    double maxDeltaTimeClamp = 0.1;  // guardrail for huge frames (s)

    // --- Free-cam movement (world m/s) ---
    double moveSpeedHorizontal = 8.0;
    double moveSpeedVertical   = 6.0;
    double freeAccelHz         = 10.0; // responsiveness of velocity filter
    double sprintMultiplier    = 1.8;
    bool   pitchAffectsForward = false;
    double freeVelDeadzone     = 1e-4; // velocity snap threshold

    // --- Free-look rotation tuning ---
    double freeLookSensYaw     = 0.0025; // radians per pixel
    double freeLookSensPitch   = 0.0020; // radians per pixel
    bool   invertFreeLookYaw   = false;   // invert X (mouse/controller)
    bool   invertFreeLookPitch = false;   // invert Y
    bool   invertLockYaw       = false;   // invert yaw offsets in lock mode
    bool   invertLockPitch     = false;   // invert pitch offsets in lock mode

    // --- Target-lock tuning ---
    double shoulderOffset   = 0.6;     // meters; >0 shifts to right shoulder
    double dynamicShoulderFactor = 0.2; // adjust shoulder based on mouse yaw offset
    double pitchBias        = -0.2;    // radians; slight down-tilt for view
    double pitchMin         = -1.45;   // ~-83°
    double pitchMax         =  1.45;   // ~+83°
    double topBlendScale    = 10.0;    // pitch stabilizes faster near vertical
    bool   clampPitch       = true;
    bool   alwaysTickFreeMode = true;  // tick even when fully unlocked (t==0)
    double nearVerticalDeg  = 2.0;     // yaw stabilization threshold (degrees)

    // --- Optional: soft ground clamp you can wire later ---
    bool   softGroundClamp  = true;   // if true, ease toward ground plane
    double groundClampHz    = 20.0;    // only used when softGroundClamp = true

    // --- Obstacle avoidance ---
    bool   enableObstacleAvoidance = false; // if true, raycast to avoid obstacles
    double obstacleMargin = 0.5;           // meters to keep from obstacles

    // --- Cut / teleport handling ---
    bool   enableTeleportHandling   = true;   // guard for the feature
    double teleportDistanceThreshold = 10.0;  // meters; jump larger than this triggers snap
    int    teleportSnapFrames        = 2;     // number of frames to skip smoothing entirely
    double teleportBlendSeconds      = 0.3;   // seconds of boosted smoothing after the snap
    double teleportBlendMinAlpha     = 0.65;  // minimum smoothing alpha while recovering

    // Sanity pass: clamp ranges and fix obviously-bad configs at runtime.
    void Validate() noexcept {
        // Make sure mins are sensible
        orbitDistance        = std::max(0.0, orbitDistance);
        minDistanceFromPlayer= std::max(0.0, minDistanceFromPlayer);
        terrainBuffer        = std::max(0.0, terrainBuffer);
        maxDeltaTimeClamp    = std::clamp(maxDeltaTimeClamp, 1e-4, 0.5);

        // Keep smoothing/frequencies non-negative
        transitionSpeed      = std::max(0.0, transitionSpeed);
        posResponsiveness    = std::max(0.0, posResponsiveness);
        rotResponsiveness    = std::max(0.0, rotResponsiveness);
        freeAccelHz          = std::max(0.0, freeAccelHz);
        groundClampHz        = std::max(0.0, groundClampHz);

        // Pitch limits: ensure min <= max and keep within near-pi/2
        if (pitchMin > pitchMax) std::swap(pitchMin, pitchMax);
        const double almostHalfPi = 0.98 * (kPI * 0.5);
        pitchMin = std::clamp(pitchMin, -almostHalfPi, 0.0);
        pitchMax = std::clamp(pitchMax,  0.0,          almostHalfPi);

        // near-vertical threshold safety
        nearVerticalDeg = std::clamp(nearVerticalDeg, 0.0, 89.9);

        // Sprint sane
        sprintMultiplier = std::max(1.0, sprintMultiplier);

        // Free-cam deadzone finite & non-negative
        if (!std::isfinite(freeVelDeadzone) || freeVelDeadzone < 0.0)
            freeVelDeadzone = 1e-4;

        // Dynamic shoulder factor reasonable
        dynamicShoulderFactor = std::clamp(dynamicShoulderFactor, -1.0, 1.0);

    // Inversion flags remain boolean, nothing to clamp but ensure deterministic values
    invertFreeLookYaw   = invertFreeLookYaw ? true : false;
    invertFreeLookPitch = invertFreeLookPitch ? true : false;
    invertLockYaw       = invertLockYaw ? true : false;
    invertLockPitch     = invertLockPitch ? true : false;

        // Teleport handling guards
        teleportDistanceThreshold = std::max(0.0, teleportDistanceThreshold);
        teleportSnapFrames        = std::max(0, teleportSnapFrames);
        teleportBlendSeconds      = std::clamp(teleportBlendSeconds, 0.0, 1.0);
        teleportBlendMinAlpha     = std::clamp(teleportBlendMinAlpha, 0.0, 1.0);
    }
};

// Light compile-time guardrails for defaults
static_assert(CameraFollowConfig{}.pitchMin < CameraFollowConfig{}.pitchMax,
              "Default pitchMin must be < pitchMax");
static_assert(CameraFollowConfig{}.sprintMultiplier >= 1.0,
              "Default sprintMultiplier must be >= 1.0");

struct CameraFollowState {
    double targetLockTransition = 0.0;
    bool   wasTargetLocked = false;

    // free-cam velocity (world space), m/s
    double freeVelX = 0.0, freeVelY = 0.0, freeVelZ = 0.0;

    // Persistent orbit angle for stable target-lock orbiting
    double orbitYaw = 0.0;
    double lockedOrbitOffset = 0.0;  // Separate offset for locked mode

    // Teleport handling state
    double lastDesiredPosX = 0.0;
    double lastDesiredPosY = 0.0;
    double lastDesiredPosZ = 0.0;
    bool   hasLastDesired   = false;
    int    teleportFramesRemaining = 0;
    double teleportBlendTimer = 0.0;
};

struct CameraFollowInput {
    // Player world-space position (meters)
    double playerX = 0.0;
    double playerY = 0.0;
    double playerZ = 0.0;

    bool   isTargetLocked = false;

    // Mouse-look deltas/offsets (radians); sign and scale handled by caller
    double mouseLookYawOffset   = 0.0;
    double mouseLookPitchOffset = 0.0;
};

} // namespace CameraFollow

// Update the camera given current state/config/input and timestep in seconds.
void UpdateTargetLockCamera(Camera& camera,
                            CameraFollow::CameraFollowState& state,
                            const CameraFollow::CameraFollowConfig& config,
                            const CameraFollow::CameraFollowInput& input,
                            double deltaTime,
                            physics::IPhysicsEngine* physicsEngine = nullptr);
