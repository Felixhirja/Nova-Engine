#include "CameraFollow.h"
#include "Camera.h"
#include "physics/PhysicsEngine.h"
#include <algorithm>
#include <cmath>
#include <iostream>

void UpdateTargetLockCamera(Camera& camera,
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
    if (shouldDebug || input.isTargetLocked) {
        std::cout << "Camera Debug - Player: (" << input.playerX << "," << input.playerY << "," << input.playerZ 
                  << ") Locked: " << (input.isTargetLocked ? "YES" : "NO")
                  << " YawOffset: " << yawInput
                  << " PitchOffset: " << pitchInput
                  << " OrbitYaw: " << state.orbitYaw 
                  << " LockedOffset: " << state.lockedOrbitOffset
                  << " EffectiveOrbit: " << effectiveOrbitYaw << std::endl;
    }
    
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
    const double posChange = std::sqrt((nx-cx)*(nx-cx) + (ny-cy)*(ny-cy) + (nz-cz)*(nz-cz));
    if (posChange > 1.0) {  // Only log if change is significant
        std::cout << "Large camera position change: " << posChange << " units" << std::endl;
        std::cout << "  From: (" << cx << "," << cy << "," << cz << ") To: (" << nx << "," << ny << "," << nz << ")" << std::endl;
        std::cout << "  Target: (" << tx << "," << ty << "," << tz << ") Alpha: " << posA << " dt: " << dt << std::endl;
    }

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
                
                // Debug output
                std::cout << "Obstacle avoidance: hit at (" << hitResult->hitX << "," << hitResult->hitY << "," << hitResult->hitZ 
                          << ") normal (" << hitResult->normalX << "," << hitResult->normalY << "," << hitResult->normalZ 
                          << ") dist=" << hitResult->distance << " margin=" << margin << std::endl;
                std::cout << "  Camera moved from (" << oldNx << "," << oldNy << "," << oldNz 
                          << ") to (" << nx << "," << ny << "," << nz << ")" << std::endl;
                
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
    const double finalChange = std::sqrt((nx-lastNx)*(nx-lastNx) + (ny-lastNy)*(ny-lastNy) + (nz-lastNz)*(nz-lastNz));
    if (finalChange > 5.0) {  // Log large jumps
        std::cout << "Camera jump detected: " << finalChange << " units to (" << nx << "," << ny << "," << nz << ")" << std::endl;
    }
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