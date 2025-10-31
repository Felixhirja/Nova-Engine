#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <fstream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "physics/PhysicsEngine.h"

// Forward declarations
class Camera;

// CameraDefaults namespace
namespace camera_defaults {
inline constexpr double kDefaultYawRadians = 0.0; // +X forward
}

// Camera class
class Camera {
public:
    static constexpr double kMinFovDegrees = 30.0;
    static constexpr double kMaxFovDegrees = 90.0;
    static constexpr double kDefaultFovDegrees = 60.0;
    static constexpr double kDefaultYawRadians = camera_defaults::kDefaultYawRadians;

    struct Basis {
        double forwardX;
        double forwardY;
        double forwardZ;
        double rightX;
        double rightY;
        double rightZ;
        double upX;
        double upY;
        double upZ;
    };

    Camera() : Camera(0.0, 0.0, 0.0, 0.0, kDefaultYawRadians, kDefaultFovDegrees) {}

    Camera(double x, double y, double z, double pitch, double yaw, double zoom)
        : x_(x), y_(y), z_(z), pitch_(pitch), yaw_(yaw), zoom_(zoom), targetZoom_(zoom) {}

    void SetPosition(double x, double y, double z) {
        x_ = x;
        y_ = y;
        z_ = z;
    }

    void SetOrientation(double pitch, double yaw) {
        pitch_ = pitch;
        yaw_ = yaw;
    }

    void SetZoom(double z) {
        zoom_ = z;
        targetZoom_ = z;
    }

    // Smoothly move camera towards target over time (lerp)
    void LerpTo(double targetX, double targetY, double targetZ, double alpha) {
        x_ += (targetX - x_) * alpha;
        y_ += (targetY - y_) * alpha;
        z_ += (targetZ - z_) * alpha;
    }

    // Instant move
    void MoveTo(double x, double y, double z) {
        x_ = x;
        y_ = y;
        z_ = z;
    }

    // Convert world coords to screen coords (for 3D, simplified orthographic approximation)
    void WorldToScreen(double wx, double wy, double wz, int screenW, int screenH, int &outX, int &outY) const {
        // Simplified: assume orthographic projection with zoom affecting scale
        double scale = zoom_ / kDefaultFovDegrees;
        outX = static_cast<int>((wx - x_) * scale + screenW / 2.0);
        outY = static_cast<int>((wy - y_) * scale + screenH / 2.0);
    }

    // Apply camera transformation to OpenGL modelview matrix
    void ApplyToOpenGL() const {
        // Assuming gluLookAt or similar; simplified
        // In real OpenGL, you'd use gluLookAt(x_, y_, z_, targetX, targetY, targetZ, upX, upY, upZ)
        // But since we have pitch/yaw, compute target
        double cosYaw = std::cos(yaw_);
        double sinYaw = std::sin(yaw_);
        double cosPitch = std::cos(pitch_);
        double sinPitch = std::sin(pitch_);
        double targetX = x_ + cosYaw * cosPitch;
        double targetY = y_ + sinPitch;
        double targetZ = z_ + sinYaw * cosPitch;
        // gluLookAt(x_, y_, z_, targetX, targetY, targetZ, 0, 1, 0);
        // But since we can't call OpenGL here, this is a placeholder
    }

    // Column-major view matrix (right, up, -forward, translation).
    std::array<double, 16> GetViewMatrix() const noexcept {
        double cosYaw = std::cos(yaw_);
        double sinYaw = std::sin(yaw_);
        double cosPitch = std::cos(pitch_);
        double sinPitch = std::sin(pitch_);

        // Forward vector
        double fx = cosYaw * cosPitch;
        double fy = sinPitch;
        double fz = sinYaw * cosPitch;

        // Right vector (cross forward with world up)
        double rx = -sinYaw;
        double ry = 0.0;
        double rz = cosYaw;

        // Up vector (cross right with forward)
        double ux = -cosYaw * sinPitch;
        double uy = cosPitch;
        double uz = -sinYaw * sinPitch;

        // View matrix: column-major
        return {
            rx, ry, rz, 0.0,
            ux, uy, uz, 0.0,
            -fx, -fy, -fz, 0.0,
            -(rx*x_ + ux*y_ + (-fx)*z_), -(ry*x_ + uy*y_ + (-fy)*z_), -(rz*x_ + uz*y_ + (-fz)*z_), 1.0
        };
    }

    // Column-major perspective projection using vertical FOV (degrees).
    std::array<double, 16> GetProjectionMatrix(double aspectRatio,
                                               double nearPlaneMeters,
                                               double farPlaneMeters) const noexcept {
        double fovRad = zoom_ * (M_PI / 180.0);
        double f = 1.0 / std::tan(fovRad / 2.0);
        double range = farPlaneMeters - nearPlaneMeters;

        return {
            f / aspectRatio, 0.0, 0.0, 0.0,
            0.0, f, 0.0, 0.0,
            0.0, 0.0, -(farPlaneMeters + nearPlaneMeters) / range, -1.0,
            0.0, 0.0, -2.0 * farPlaneMeters * nearPlaneMeters / range, 0.0
        };
    }

    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }
    double pitch() const { return pitch_; }
    double yaw() const { return yaw_; }
    double zoom() const { return zoom_; }
    double targetZoom() const { return targetZoom_; }
    void SetTargetZoom(double z) { targetZoom_ = z; }
    // Update zoom towards target, dt in seconds
    void UpdateZoom(double dt) {
        double alpha = 1.0 - std::exp(-10.0 * dt); // arbitrary smoothing
        zoom_ += (targetZoom_ - zoom_) * alpha;
    }

    // Build orthonormal basis vectors representing the camera rig in world space.
    // When includePitchInForward is false, the forward basis ignores pitch for horizontal moves.
    Basis BuildBasis(bool includePitchInForward = true) const noexcept {
        double cosYaw = std::cos(yaw_);
        double sinYaw = std::sin(yaw_);
        double cosPitch = std::cos(pitch_);
        double sinPitch = std::sin(pitch_);

        Basis basis;
        if (includePitchInForward) {
            basis.forwardX = cosYaw * cosPitch;
            basis.forwardY = sinPitch;
            basis.forwardZ = sinYaw * cosPitch;
        } else {
            basis.forwardX = cosYaw;
            basis.forwardY = 0.0;
            basis.forwardZ = sinYaw;
        }

        // Right = cross(forward, world up)
        basis.rightX = -basis.forwardZ;
        basis.rightY = 0.0;
        basis.rightZ = basis.forwardX;

        // Up = cross(right, forward)
        basis.upX = -cosYaw * sinPitch;
        basis.upY = cosPitch;
        basis.upZ = -sinYaw * sinPitch;

        return basis;
    }

private:
    static double ClampFov(double fov) noexcept {
        return std::clamp(fov, kMinFovDegrees, kMaxFovDegrees);
    }

    double x_;
    double y_;
    double z_;
    double pitch_;
    double yaw_;
    double zoom_;
    double targetZoom_;
};

// CameraFollow namespace
namespace CameraFollow {

// --- Small math helpers (header-only, no cost with ODR-use) ---
constexpr double kPI  = 3.14159265358979323846;
constexpr double kTAU = 2.0 * kPI;

constexpr double DegToRad(double d) noexcept { return d * (kPI / 180.0); }
constexpr double RadToDeg(double r) noexcept { return r * (180.0 / kPI); }

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
    double freeLookSensYaw     = 0.01;  // radians per pixel (increased from 0.0025 for better responsiveness)
    double freeLookSensPitch   = 0.008; // radians per pixel (increased from 0.0020 for better responsiveness)
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
    double lockedOrbitOffset = 0.0;  // Planar angle from player to camera when locked

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
                            double dt,
                            physics::IPhysicsEngine* physicsEngine = nullptr);

// CameraFollowController
struct CameraMovementInput {
    bool moveForward = false;
    bool moveBackward = false;
    bool moveLeft = false;
    bool moveRight = false;
    bool moveUp = false;
    bool moveDown = false;
    double moveSpeed = 0.5;
    bool sprint = false;
    bool slow = false;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
};

class CameraFollowController {
public:
    void SetConfig(const CameraFollow::CameraFollowConfig& config);
    const CameraFollow::CameraFollowConfig& GetConfig() const { return config_; }

    const CameraFollow::CameraFollowState& GetState() const { return state_; }
    void ResetState();
    void SuppressNextUpdate() { suppressNextUpdate_ = true; }

    void Update(class Camera& camera,
                const CameraFollow::CameraFollowInput& followInput,
                const CameraMovementInput& movementInput,
                double deltaTime,
                physics::IPhysicsEngine* physicsEngine = nullptr);

private:
    CameraFollow::CameraFollowConfig config_;
    CameraFollow::CameraFollowState state_;
    bool suppressNextUpdate_ = false;

    void ApplyFreeLookRotation(class Camera& camera,
                               const CameraMovementInput& movementInput,
                               const CameraFollow::CameraFollowConfig& config,
                               double deltaTime);

    void ApplyFreeCameraMovement(class Camera& camera,
                                 const CameraFollow::CameraFollowInput& followInput,
                                 const CameraMovementInput& movementInput,
                                 const CameraFollow::CameraFollowConfig& config,
                                 double deltaTime);
};

// CameraPresets
struct CameraPreset {
    double x;
    double y;
    double z;
    double pitch;
    double yaw;
    double zoom;
};

const std::array<CameraPreset, 3>& GetDefaultCameraPresets();

void ApplyPresetToCamera(Camera& camera, const CameraPreset& preset);

// CameraConfigLoader
namespace CameraConfigLoader {

// Loads the camera follow configuration from an INI-style profile file.
// Returns true when a profile is found and applied; on failure outConfig is left untouched.
bool LoadCameraFollowConfigProfile(const std::string& path,
                                   const std::string& profileName,
                                   CameraFollow::CameraFollowConfig& outConfig);

// Convenience overload that loads the "default" profile.
bool LoadCameraFollowConfig(const std::string& path,
                            CameraFollow::CameraFollowConfig& outConfig);

} // namespace CameraConfigLoader

// Implementations

// CameraFollowController implementations

inline void CameraFollowController::SetConfig(const CameraFollow::CameraFollowConfig& config) {
    config_ = config;
}

inline void CameraFollowController::ResetState() {
    state_ = CameraFollow::CameraFollowState{};
}

inline void CameraFollowController::Update(Camera& camera,
                                    const CameraFollow::CameraFollowInput& followInput,
                                    const CameraMovementInput& movementInput,
                                    double deltaTime,
                                    physics::IPhysicsEngine* physicsEngine) {
    if (suppressNextUpdate_) {
        suppressNextUpdate_ = false;
        return;
    }

    // Handoff: zero velocity when entering target-lock
    if (followInput.isTargetLocked && !state_.wasTargetLocked) {
        state_.freeVelX = state_.freeVelY = state_.freeVelZ = 0.0;
        suppressNextUpdate_ = true; // absorb one frame of inputs if needed
    }

    // Instant transition when exiting target-lock for immediate mouse look
    if (!followInput.isTargetLocked && state_.wasTargetLocked) {
        state_.targetLockTransition = 0.0;
    }

    UpdateTargetLockCamera(camera, state_, config_, followInput, deltaTime, physicsEngine);

    if (!followInput.isTargetLocked && state_.targetLockTransition <= 0.0) {
        ApplyFreeLookRotation(camera, movementInput, config_, deltaTime);
        ApplyFreeCameraMovement(camera, followInput, movementInput, config_, deltaTime);
    }

    state_.wasTargetLocked = followInput.isTargetLocked;
}

inline void CameraFollowController::ApplyFreeCameraMovement(Camera& camera,
                                                     const CameraFollow::CameraFollowInput& /*followInput*/,
                                                     const CameraMovementInput& movementInput,
                                                     const CameraFollow::CameraFollowConfig& config,
                                                     double deltaTime) {
    // Early out if no base speed
    if (movementInput.moveSpeed <= 0.0 &&
        config.moveSpeedHorizontal <= 0.0 &&
        config.moveSpeedVertical   <= 0.0) return;

    const double dt = std::clamp(deltaTime, 0.0, config.maxDeltaTimeClamp);

    // Sprint/slow factor
    const double speedFactor =
        (movementInput.sprint ? config.sprintMultiplier : 1.0) *
        (movementInput.slow   ? 0.5                      : 1.0);

    // --- Build camera basis (X right, Y up, Z forward) ---
    const double yaw   = camera.yaw();
    const double pitch = camera.pitch();

    const Camera::Basis basis = camera.BuildBasis(config.pitchAffectsForward);
    const double fwdX = basis.forwardX;
    const double fwdY = basis.forwardY;
    const double fwdZ = basis.forwardZ;
    const double rightX = basis.rightX;
    const double rightY = basis.rightY;
    const double rightZ = basis.rightZ;

    // Input axes (−1..+1)
    const int fwdIn   = (movementInput.moveForward ? 1 : 0) - (movementInput.moveBackward ? 1 : 0);
    const int rightIn = (movementInput.moveRight   ? 1 : 0) - (movementInput.moveLeft     ? 1 : 0);
    const int upIn    = (movementInput.moveUp      ? 1 : 0) - (movementInput.moveDown     ? 1 : 0);

    // --- Desired velocity (split horizontal XZ and vertical Y) ---
    // Horizontal from right/fwd (XZ only)
    double vxH = rightIn * rightX + fwdIn * fwdX;
    double vzH = rightIn * rightZ + fwdIn * fwdZ;

    // Normalize horizontal to avoid faster diagonals
    const double horizLen2 = vxH*vxH + vzH*vzH;
    if (horizLen2 > 0.0) {
        const double inv = 1.0 / std::sqrt(horizLen2);
        vxH *= inv; vzH *= inv;
    }

    // Vertical from world up (Y) — independent of horizontal magnitude
    const double vyV = static_cast<double>(upIn);

    // Select speeds
    const double baseH = (movementInput.moveSpeed > 0.0 ? movementInput.moveSpeed : config_.moveSpeedHorizontal);
    const double baseV = (movementInput.moveSpeed > 0.0 ? movementInput.moveSpeed : config_.moveSpeedVertical);

    // Scale by speeds and sprint/slow
    double desVelX = vxH * baseH * speedFactor;
    double desVelZ = vzH * baseH * speedFactor;
    double desVelY = vyV * baseV * speedFactor;

    // Exponential velocity smoothing
    auto expAlpha = [](double hz, double dtSec){
        const double lambda = std::max(0.0, hz);
        return 1.0 - std::exp(-lambda * std::max(0.0, dtSec));
    };
    const double velAlpha = std::clamp(expAlpha(config.freeAccelHz, dt), 0.0, 1.0);

    // If no input, gently damp toward zero to kill drift
    const bool noInput = (fwdIn == 0) && (rightIn == 0) && (upIn == 0);
    if (noInput) {
        state_.freeVelX += (-state_.freeVelX) * velAlpha;
        state_.freeVelY += (-state_.freeVelY) * velAlpha;
        state_.freeVelZ += (-state_.freeVelZ) * velAlpha;
    } else {
        state_.freeVelX += (desVelX - state_.freeVelX) * velAlpha;
        state_.freeVelY += (desVelY - state_.freeVelY) * velAlpha;
        state_.freeVelZ += (desVelZ - state_.freeVelZ) * velAlpha;
    }

    // Deadzone snap
    auto snapZero = [&](double& v){ if (std::abs(v) < config.freeVelDeadzone) v = 0.0; };
    snapZero(state_.freeVelX);
    snapZero(state_.freeVelY);
    snapZero(state_.freeVelZ);

    // Integrate
    camera.SetPosition(camera.x() + state_.freeVelX * dt,
                       camera.y() + state_.freeVelY * dt,
                       camera.z() + state_.freeVelZ * dt);
}

inline void CameraFollowController::ApplyFreeLookRotation(Camera& camera,
                                                    const CameraMovementInput& movementInput,
                                                    const CameraFollow::CameraFollowConfig& config,
                                                    double /*deltaTime*/) {
    // Sensitivity: radians per pixel
    const double sensYaw   = config.freeLookSensYaw;
    const double sensPitch = config.freeLookSensPitch;

    // Optional: tiny deadzone on mouse input to stop shimmer
    auto deadzone = [](double v, double dz){ return (std::abs(v) < dz) ? 0.0 : v; };
    const double dzPx = 0.2; // tweak
    double dx = deadzone(movementInput.mouseDeltaX, dzPx);
    double dy = deadzone(movementInput.mouseDeltaY, dzPx);

    // Get current orientation
    double yaw   = camera.yaw();
    double pitch = camera.pitch();

    // Apply mouse deltas (note: mouseDeltaY typically positive downward, so invert for pitch)
    const double yawSign   = config.invertFreeLookYaw   ? -1.0 : 1.0;
    const double pitchSign = config.invertFreeLookPitch ? -1.0 : 1.0;

    yaw   += yawSign   * dx * sensYaw;
    double pitchDelta = -dy * sensPitch;
    pitch += pitchSign * pitchDelta;

    // Wrap yaw to avoid unbounded growth
    yaw = std::remainder(yaw, 2.0 * CameraFollow::kPI);

    // Clamp pitch to prevent gimbal lock (approx -89° to +89°)
    const double maxPitch = 1.55334; // ~89° in radians
    pitch = std::clamp(pitch, -maxPitch, maxPitch);

    // Set new orientation
    camera.SetOrientation(pitch, yaw);
}

// CameraPresets implementations
namespace {
const std::array<CameraPreset, 3> kDefaultPresets = {{
    // 1: Default orbit behind player
    {-8.0, 0.0, 6.0, -0.1, Camera::kDefaultYawRadians, 60.0},
    // 2: High top-down overview
    {0.0, -12.0, 18.0, -1.2, Camera::kDefaultYawRadians, 75.0},
    // 3: Cinematic side angle
    {15.0, 5.0, 6.0, -0.25, -1.2, 55.0}
}};
}

inline const std::array<CameraPreset, 3>& GetDefaultCameraPresets() {
    return kDefaultPresets;
}

inline void ApplyPresetToCamera(Camera& camera, const CameraPreset& preset) {
    camera.SetPosition(preset.x, preset.y, preset.z);
    camera.SetOrientation(preset.pitch, preset.yaw);
    camera.SetZoom(preset.zoom);
    camera.SetTargetZoom(preset.zoom);
}

// CameraConfigLoader implementations
namespace {

std::string Trim(const std::string& value) {
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

bool ParseDouble(const std::string& text, double& outValue) {
    errno = 0;
    char* endPtr = nullptr;
    const char* begin = text.c_str();
    double value = std::strtod(begin, &endPtr);
    if (begin == endPtr || errno == ERANGE) {
        return false;
    }
    while (endPtr && *endPtr != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*endPtr))) {
            return false;
        }
        ++endPtr;
    }
    outValue = value;
    return true;
}

bool ParseInt(const std::string& text, int& outValue) {
    errno = 0;
    char* endPtr = nullptr;
    const char* begin = text.c_str();
    long value = std::strtol(begin, &endPtr, 10);
    if (begin == endPtr || errno == ERANGE) {
        return false;
    }
    while (endPtr && *endPtr != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*endPtr))) {
            return false;
        }
        ++endPtr;
    }
    outValue = static_cast<int>(value);
    return true;
}

bool ParseBool(const std::string& text, bool& outValue) {
    std::string lower;
    lower.reserve(text.size());
    for (char c : text) {
        if (!std::isspace(static_cast<unsigned char>(c))) {
            lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
        }
    }
    if (lower == "true" || lower == "1" || lower == "yes" || lower == "on") {
        outValue = true;
        return true;
    }
    if (lower == "false" || lower == "0" || lower == "no" || lower == "off") {
        outValue = false;
        return true;
    }
    return false;
}

bool ApplyKeyValue(CameraFollow::CameraFollowConfig& config,
                   const std::string& key,
                   const std::string& value) {
    double numeric = 0.0;
    bool boolValue = false;
    int intValue = 0;

    if (key == "orbitDistance" && ParseDouble(value, numeric)) {
        config.orbitDistance = numeric;
    } else if (key == "orbitHeight" && ParseDouble(value, numeric)) {
        config.orbitHeight = numeric;
    } else if (key == "minDistanceFromPlayer" && ParseDouble(value, numeric)) {
        config.minDistanceFromPlayer = numeric;
    } else if (key == "groundLevel" && ParseDouble(value, numeric)) {
        config.groundLevel = numeric;
    } else if (key == "terrainBuffer" && ParseDouble(value, numeric)) {
        config.terrainBuffer = numeric;
    } else if (key == "transitionSpeed" && ParseDouble(value, numeric)) {
        config.transitionSpeed = numeric;
    } else if (key == "posResponsiveness" && ParseDouble(value, numeric)) {
        config.posResponsiveness = numeric;
    } else if (key == "rotResponsiveness" && ParseDouble(value, numeric)) {
        config.rotResponsiveness = numeric;
    } else if (key == "maxDeltaTimeClamp" && ParseDouble(value, numeric)) {
        config.maxDeltaTimeClamp = numeric;
    } else if (key == "moveSpeedHorizontal" && ParseDouble(value, numeric)) {
        config.moveSpeedHorizontal = numeric;
    } else if (key == "moveSpeedVertical" && ParseDouble(value, numeric)) {
        config.moveSpeedVertical = numeric;
    } else if (key == "freeAccelHz" && ParseDouble(value, numeric)) {
        config.freeAccelHz = numeric;
    } else if (key == "sprintMultiplier" && ParseDouble(value, numeric)) {
        config.sprintMultiplier = numeric;
    } else if (key == "pitchAffectsForward" && ParseBool(value, boolValue)) {
        config.pitchAffectsForward = boolValue;
    } else if (key == "freeVelDeadzone" && ParseDouble(value, numeric)) {
        config.freeVelDeadzone = numeric;
    } else if (key == "freeLookSensYaw" && ParseDouble(value, numeric)) {
        config.freeLookSensYaw = numeric;
    } else if (key == "freeLookSensPitch" && ParseDouble(value, numeric)) {
        config.freeLookSensPitch = numeric;
    } else if (key == "invertFreeLookYaw" && ParseBool(value, boolValue)) {
        config.invertFreeLookYaw = boolValue;
    } else if (key == "invertFreeLookPitch" && ParseBool(value, boolValue)) {
        config.invertFreeLookPitch = boolValue;
    } else if (key == "invertLockYaw" && ParseBool(value, boolValue)) {
        config.invertLockYaw = boolValue;
    } else if (key == "invertLockPitch" && ParseBool(value, boolValue)) {
        config.invertLockPitch = boolValue;
    } else if (key == "shoulderOffset" && ParseDouble(value, numeric)) {
        config.shoulderOffset = numeric;
    } else if (key == "dynamicShoulderFactor" && ParseDouble(value, numeric)) {
        config.dynamicShoulderFactor = numeric;
    } else if (key == "pitchBias" && ParseDouble(value, numeric)) {
        config.pitchBias = numeric;
    } else if (key == "pitchMin" && ParseDouble(value, numeric)) {
        config.pitchMin = numeric;
    } else if (key == "pitchMax" && ParseDouble(value, numeric)) {
        config.pitchMax = numeric;
    } else if (key == "topBlendScale" && ParseDouble(value, numeric)) {
        config.topBlendScale = numeric;
    } else if (key == "clampPitch" && ParseBool(value, boolValue)) {
        config.clampPitch = boolValue;
    } else if (key == "alwaysTickFreeMode" && ParseBool(value, boolValue)) {
        config.alwaysTickFreeMode = boolValue;
    } else if (key == "nearVerticalDeg" && ParseDouble(value, numeric)) {
        config.nearVerticalDeg = numeric;
    } else if (key == "softGroundClamp" && ParseBool(value, boolValue)) {
        config.softGroundClamp = boolValue;
    } else if (key == "groundClampHz" && ParseDouble(value, numeric)) {
        config.groundClampHz = numeric;
    } else if (key == "enableObstacleAvoidance" && ParseBool(value, boolValue)) {
        config.enableObstacleAvoidance = boolValue;
    } else if (key == "obstacleMargin" && ParseDouble(value, numeric)) {
        config.obstacleMargin = numeric;
    } else if (key == "enableTeleportHandling" && ParseBool(value, boolValue)) {
        config.enableTeleportHandling = boolValue;
    } else if (key == "teleportDistanceThreshold" && ParseDouble(value, numeric)) {
        config.teleportDistanceThreshold = numeric;
    } else if (key == "teleportSnapFrames" && ParseInt(value, intValue)) {
        config.teleportSnapFrames = intValue;
    } else if (key == "teleportBlendSeconds" && ParseDouble(value, numeric)) {
        config.teleportBlendSeconds = numeric;
    } else if (key == "teleportBlendMinAlpha" && ParseDouble(value, numeric)) {
        config.teleportBlendMinAlpha = numeric;
    } else {
        return false;
    }
    return true;
}

bool ParseCameraConfigStream(std::istream& input,
                             std::unordered_map<std::string, CameraFollow::CameraFollowConfig>& outProfiles) {
    std::string line;
    std::string currentProfile;
    CameraFollow::CameraFollowConfig currentConfig;
    bool inProfile = false;

    auto commit = [&]() {
        if (!currentProfile.empty()) {
            currentConfig.Validate();
            outProfiles[currentProfile] = currentConfig;
        }
    };

    while (std::getline(input, line)) {
        std::string trimmed = Trim(line);
        if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
            continue;
        }

        if (trimmed.front() == '[' && trimmed.back() == ']') {
            if (inProfile) {
                commit();
            }
            currentProfile = Trim(trimmed.substr(1, trimmed.size() - 2));
            currentConfig = CameraFollow::CameraFollowConfig{};
            inProfile = true;
            continue;
        }

        auto equalsPos = trimmed.find('=');
        if (equalsPos == std::string::npos || !inProfile) {
            continue;
        }

        std::string key = Trim(trimmed.substr(0, equalsPos));
        std::string value = Trim(trimmed.substr(equalsPos + 1));
        if (key.empty()) {
            continue;
        }
        ApplyKeyValue(currentConfig, key, value);
    }

    if (inProfile) {
        commit();
    }

    return !outProfiles.empty();
}

bool IsRelativePath(const std::string& path) {
    if (path.empty()) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return false;
    }
    if (path.size() > 1 && path[1] == ':') {
        return false;
    }
    return true;
}

bool LoadProfiles(const std::string& path,
                  std::unordered_map<std::string, CameraFollow::CameraFollowConfig>& outProfiles) {
    std::vector<std::string> candidates;
    candidates.push_back(path);
    if (IsRelativePath(path)) {
        candidates.push_back("../" + path);
        candidates.push_back("../../" + path);
    }

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (!file.is_open()) {
            continue;
        }
        std::unordered_map<std::string, CameraFollow::CameraFollowConfig> parsed;
        if (ParseCameraConfigStream(file, parsed)) {
            outProfiles = std::move(parsed);
            return true;
        }
    }
    return false;
}

} // namespace

namespace CameraConfigLoader {

inline bool LoadCameraFollowConfigProfile(const std::string& path,
                                   const std::string& profileName,
                                   CameraFollow::CameraFollowConfig& outConfig) {
    std::unordered_map<std::string, CameraFollow::CameraFollowConfig> profiles;
    if (!LoadProfiles(path, profiles)) {
        return false;
    }

    auto it = profiles.find(profileName);
    if (it != profiles.end()) {
        outConfig = it->second;
        return true;
    }

    auto defaultIt = profiles.find("default");
    if (defaultIt != profiles.end()) {
        outConfig = defaultIt->second;
        return true;
    }

    if (!profiles.empty()) {
        outConfig = profiles.begin()->second;
        return true;
    }

    return false;
}

inline bool LoadCameraFollowConfig(const std::string& path,
                            CameraFollow::CameraFollowConfig& outConfig) {
    return LoadCameraFollowConfigProfile(path, "default", outConfig);
}

} // namespace CameraConfigLoader

// UpdateTargetLockCamera implementation
inline void UpdateTargetLockCamera(Camera& camera,
                            CameraFollow::CameraFollowState& state,
                            const CameraFollow::CameraFollowConfig& config,
                            const CameraFollow::CameraFollowInput& input,
                            double dt,
                            physics::IPhysicsEngine* physicsEngine)
{
    using std::clamp;
    using std::remainder;

    // Validate config at runtime to catch bad hot-loads
    const_cast<CameraFollow::CameraFollowConfig&>(config).Validate();

    constexpr double kTAU       = CameraFollow::kTAU;
    constexpr double kEps       = 1e-6;

    // Clamp dt safely even if config has nonsense
    dt = clamp(dt, 0.0, std::max(0.0, config.maxDeltaTimeClamp));

    const bool teleportEnabled = config.enableTeleportHandling;
    const bool hadLastDesired = state.hasLastDesired;
    const double prevDesiredPosX = state.lastDesiredPosX;
    const double prevDesiredPosY = state.lastDesiredPosY;
    const double prevDesiredPosZ = state.lastDesiredPosZ;

    auto triggerTeleportRecovery = [&]() {
        if (!teleportEnabled) {
            return;
        }
        if (config.teleportSnapFrames > 0) {
            state.teleportFramesRemaining = std::max(state.teleportFramesRemaining, config.teleportSnapFrames);
        }
        if (config.teleportBlendSeconds > 0.0) {
            state.teleportBlendTimer = std::max(state.teleportBlendTimer, config.teleportBlendSeconds);
        }
        state.freeVelX = state.freeVelY = state.freeVelZ = 0.0;
    };

    // Debug: Log input values periodically (every ~5 seconds)
    static double debugTimer = 0.0;
    debugTimer += dt;
    bool shouldDebug = debugTimer > 5.0;
    if (shouldDebug) {
        debugTimer = 0.0;
        // Debug output moved after effectiveOrbitYaw is calculated
    }

    // --- Transition t in [0,1] with exponential smoothing ---
    const double tTarget = input.isTargetLocked ? 1.0 : 0.0;
    const double tA      = clamp(CameraFollow::ExpAlpha(config.transitionSpeed, dt), 0.0, 1.0);
    state.targetLockTransition += (tTarget - state.targetLockTransition) * tA;
    const double t = CameraFollow::SmoothStep(clamp(state.targetLockTransition, 0.0, 1.0));

    // Optional: keep ticking even when unlocked
    if (!config.alwaysTickFreeMode) {
        if (t <= 0.0 && !input.isTargetLocked) return;
    }

    const double yawInputRaw = input.mouseLookYawOffset;
    const double pitchInputRaw = input.mouseLookPitchOffset;
    const double yawInput = config.invertLockYaw ? -yawInputRaw : yawInputRaw;
    const double pitchInput = config.invertLockPitch ? -pitchInputRaw : pitchInputRaw;

    // --- Player and current cam ---
    const double px = input.playerX, py = input.playerY, pz = input.playerZ;
    const double cx = camera.x(),    cy = camera.y(),    cz = camera.z();
    double camYaw   = camera.yaw();
    double camPitch = camera.pitch();

    // Bound yaw every frame
    camYaw = std::remainder(camYaw, kTAU);

    // --- Desired locked orbit (XZ plane, Y up) ---
    // Use separate orbit offset for locked mode to avoid accumulation issues
    double effectiveOrbitYaw = 0.0;
    double yawForShoulder = 0.0;

    if (input.isTargetLocked) {
        // When locking, capture the current planar angle from player to camera so we keep continuity.
        if (!state.wasTargetLocked) {
            const double dxCam = px - cx;
            const double dzCam = pz - cz;
            const double planarLenSq = dxCam * dxCam + dzCam * dzCam;
            if (planarLenSq > (kEps * kEps)) {
                state.lockedOrbitOffset = std::atan2(dxCam, dzCam);
            } else {
                state.lockedOrbitOffset = camYaw;
            }
            state.lockedOrbitOffset = remainder(state.lockedOrbitOffset, kTAU);
        }

        // For locked mode, treat mouseLookYawOffset as a delta rather than accumulated offset.
        // This assumes the caller resets mouseLookYawOffset each frame when locked.
        double mouseDeltaYaw = yawInput;
        
        // Only accumulate if there's significant mouse movement
        if (std::abs(mouseDeltaYaw) > 0.001) { // Increased threshold to prevent drift
            // Clamp delta to prevent large jumps
            mouseDeltaYaw = std::clamp(mouseDeltaYaw, -0.1, 0.1); // Max 0.1 radians per frame
            state.lockedOrbitOffset += mouseDeltaYaw;
        }
        
        // Keep in reasonable range
        state.lockedOrbitOffset = std::remainder(state.lockedOrbitOffset, 2.0 * CameraFollow::kPI);
        effectiveOrbitYaw = state.lockedOrbitOffset;
        yawForShoulder = mouseDeltaYaw;
    } else {
        // When unlocked, sync orbit yaw to current camera yaw for smooth transition
        state.orbitYaw = remainder(camYaw, kTAU);
        effectiveOrbitYaw = state.orbitYaw;
        yawForShoulder = 0.0;
    }
    
    // Update wasTargetLocked for next frame
    state.wasTargetLocked = input.isTargetLocked;
    
    // Debug output after effectiveOrbitYaw is calculated
    // Disabled for performance - was printing every frame
    // if (shouldDebug) {  
    //     std::cout << "Camera Debug - Player: (" << input.playerX << "," << input.playerY << "," << input.playerZ 
    //               << ") Locked: " << (input.isTargetLocked ? "YES" : "NO")
    //               << " YawOffset: " << yawInput
    //               << " PitchOffset: " << pitchInput
    //               << " OrbitYaw: " << state.orbitYaw 
    //               << " LockedOffset: " << state.lockedOrbitOffset
    //               << " EffectiveOrbit: " << effectiveOrbitYaw << std::endl;
    // }
    
    const double s = std::sin(effectiveOrbitYaw);
    const double c = std::cos(effectiveOrbitYaw);

    double lockX = px - s * config.orbitDistance;
    double lockZ = pz - c * config.orbitDistance;
    double lockY = py + config.orbitHeight;

    // Shoulder offset (third-person "over the shoulder")
    // Dynamic adjustment based on mouse yaw offset to keep target visible
    const double baseShoulder = config.shoulderOffset;
    const double dynamicAdjust = yawForShoulder * config.dynamicShoulderFactor;
    const double shoulder = std::clamp(baseShoulder - dynamicAdjust, -2.0, 2.0); // reasonable bounds
    const double rightX = c, rightZ = -s;
    lockX += rightX * shoulder;
    lockZ += rightZ * shoulder;

    // --- Blend between free (current) and locked ---
    const double tx = cx + (lockX - cx) * t;
    const double ty = cy + (lockY - cy) * t;
    const double tz = cz + (lockZ - cz) * t;

    if (teleportEnabled && hadLastDesired) {
        const double dpx = tx - prevDesiredPosX;
        const double dpy = ty - prevDesiredPosY;
        const double dpz = tz - prevDesiredPosZ;
        const double jumpDistance = std::sqrt(dpx * dpx + dpy * dpy + dpz * dpz);
        if (jumpDistance > config.teleportDistanceThreshold) {
            triggerTeleportRecovery();
        }
    }

    // --- Frame-independent smoothing ---
    double posA = clamp(CameraFollow::ExpAlpha(config.posResponsiveness, dt), 0.0, 1.0);
    double rotA = clamp(CameraFollow::ExpAlpha(config.rotResponsiveness, dt), 0.0, 1.0);

    if (teleportEnabled) {
        if (state.teleportFramesRemaining > 0) {
            posA = 1.0;
            rotA = 1.0;
        } else if (state.teleportBlendTimer > 0.0) {
            posA = std::max(posA, config.teleportBlendMinAlpha);
            rotA = std::max(rotA, config.teleportBlendMinAlpha);
        }
    }

    // --- Position (smooth toward blended target) ---
    double nx = cx + (tx - cx) * posA;
    double ny = cy + (ty - cy) * posA;
    double nz = cz + (tz - cz) * posA;

    // Debug: Log significant position changes
    // Disabled frequent debug output for performance
    // if (posChange > 1.0) {  
    //     std::cout << "Large camera position change: " << posChange << " units" << std::endl;
    //     std::cout << "  From: (" << cx << "," << cy << "," << cz << ") To: (" << nx << "," << ny << "," << nz << ")" << std::endl;
    //     std::cout << "  Target: (" << tx << "," << ty << "," << tz << ") Alpha: " << posA << " dt: " << dt << std::endl;
    // }

    // --- Enforce min distance from player (guard zero) ---
    {
        const double dxp = nx - px;
        const double dyp = ny - py;
        const double dzp = nz - pz;
        const double dist = std::sqrt(std::max(0.0, dxp*dxp + dyp*dyp + dzp*dzp));
        if (dist > kEps && dist < config.minDistanceFromPlayer) {
            const double k = config.minDistanceFromPlayer / dist;
            nx = px + dxp * k;
            ny = py + dyp * k;
            nz = pz + dzp * k;
        }
    }

    // --- Ground clamp AFTER min-distance push ---
    const double groundY = config.groundLevel + config.terrainBuffer;
    if (config.softGroundClamp && ny < groundY) {
        // Soft spring-based easing toward ground
        const double groundA = clamp(CameraFollow::ExpAlpha(config.groundClampHz, dt), 0.0, 1.0);
        ny += (groundY - ny) * groundA;
    } else {
        // Hard clamp as fallback
        ny = std::max(ny, groundY);
    }

    // --- Obstacle avoidance ---
    if (config.enableObstacleAvoidance && physicsEngine) {
        // Raycast from player to desired camera position
        const double rayOriginX = px;
        const double rayOriginY = py;
        const double rayOriginZ = pz;
        const double rayDirX = nx - px;
        const double rayDirY = ny - py;
        const double rayDirZ = nz - pz;
        const double rayLength = std::sqrt(rayDirX*rayDirX + rayDirY*rayDirY + rayDirZ*rayDirZ);
        
        if (rayLength > 1e-6) {
            // Normalize direction
            const double invLength = 1.0 / rayLength;
            const double normDirX = rayDirX * invLength;
            const double normDirY = rayDirY * invLength;
            const double normDirZ = rayDirZ * invLength;
            
            // Perform raycast
            auto hitResult = physicsEngine->Raycast(rayOriginX, rayOriginY, rayOriginZ,
                                                   normDirX, normDirY, normDirZ,
                                                   rayLength);
            
            if (hitResult) {
                // Hit detected - move camera to hit point + normal * margin
                const double margin = config.obstacleMargin;
                const double oldNx = nx, oldNy = ny, oldNz = nz;
                nx = hitResult->hitX + hitResult->normalX * margin;
                ny = hitResult->hitY + hitResult->normalY * margin;
                nz = hitResult->hitZ + hitResult->normalZ * margin;
                
                // Debug output - disabled for performance
                // std::cout << "Obstacle avoidance: hit at (" << hitResult->hitX << "," << hitResult->hitY << "," << hitResult->hitZ 
                //           << ") normal (" << hitResult->normalX << "," << hitResult->normalY << "," << hitResult->normalZ 
                //           << ") dist=" << hitResult->distance << " margin=" << margin << std::endl;
                // std::cout << "  Camera moved from (" << oldNx << "," << oldNy << "," << oldNz 
                //           << ") to (" << nx << "," << ny << "," << nz << ")" << std::endl;
                
                // Ensure we don't go below ground after obstacle adjustment
                ny = std::max(ny, groundY);
            }
        }
    }

    // --- Orientation: look at player (mouse pitch affects aim, not height) ---
    const double dx = px - nx;
    const double dz = pz - nz;
    const double dy = py - ny;

    const double horizRaw = std::hypot(dx, dz);
    const double horiz    = std::max(kEps, horizRaw);

    // Angle-based "near vertical" check (in radians)
    const double elev = std::atan2(dy, horiz); // 0 on horizon, +/- near vertical
    const double nearVertRad = clamp(config.nearVerticalDeg, 0.0, 89.9) * (CameraFollow::kPI / 180.0);
    const bool   nearVertical = std::abs(elev) > (CameraFollow::kPI / 2.0 - nearVertRad);

    // yawToTarget = atan2(dx, dz) assumes +Z-forward / yaw=0 aligns with +Z
    const double yawToTarget = std::atan2(dx, dz);
    const double yawLocked   = nearVertical ? camYaw : yawToTarget;

    const double pitchLocked = -std::atan2(dy, horiz) + config.pitchBias + (pitchInput * t);

    // Blend orientation target by t (shortest-arc yaw), then smooth by rotA
    double targetYaw   = camYaw   + remainder(yawLocked - camYaw, kTAU) * t;
    double targetPitch = camPitch + (pitchLocked - camPitch) * t;

    // Softer near-top pitch blend, scaled from config
    const double topBlend = clamp(horizRaw * config.topBlendScale, 0.0, 1.0);
    targetPitch = camPitch + (pitchLocked - camPitch) * (t * topBlend);


    camYaw   = camYaw   + remainder(targetYaw - camYaw, kTAU) * rotA;
    camPitch = camPitch + (targetPitch - camPitch) * rotA;

    // Optional pitch clamp
    if (config.clampPitch) {
        camPitch = clamp(camPitch, config.pitchMin, config.pitchMax);
    }

    // Keep yaw bounded
    camYaw = std::remainder(camYaw, kTAU);

    camera.SetOrientation(camPitch, camYaw);

    camera.SetPosition(nx, ny, nz);

    // Debug: Log final camera position
    static double lastNx = 0.0, lastNy = 0.0, lastNz = 0.0;
    // Disabled frequent debug output for performance
    // if (finalChange > 5.0) {  // Log large jumps
    //     std::cout << "Camera jump detected: " << finalChange << " units to (" << nx << "," << ny << "," << nz << ")" << std::endl;
    // }
    lastNx = nx; lastNy = ny; lastNz = nz;

    if (teleportEnabled) {
        if (state.teleportFramesRemaining > 0) {
            state.teleportFramesRemaining = std::max(0, state.teleportFramesRemaining - 1);
        } else if (state.teleportBlendTimer > 0.0) {
            state.teleportBlendTimer = std::max(0.0, state.teleportBlendTimer - dt);
        }
    }

    state.lastDesiredPosX = tx;
    state.lastDesiredPosY = ty;
    state.lastDesiredPosZ = tz;
    state.hasLastDesired = true;
}