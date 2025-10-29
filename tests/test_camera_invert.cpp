#include "../engine/CameraSystem.h"

#include <cassert>
#include <iostream>

int main() {
    const double dt = 1.0 / 60.0;

    CameraFollowController controller;
    Camera camera;

    CameraMovementInput movement{};
    movement.mouseDeltaX = 6.0;
    movement.mouseDeltaY = 4.0;

    CameraFollow::CameraFollowInput followInput{};
    followInput.isTargetLocked = false;

    CameraFollow::CameraFollowConfig config;
    config.alwaysTickFreeMode = false; // Skip target-lock blending when unlocked.
    config.freeLookSensYaw = 0.01;
    config.freeLookSensPitch = 0.01;

    controller.SetConfig(config);
    controller.ResetState();

    controller.Update(camera, followInput, movement, dt, nullptr);
    const double yawDefault = camera.yaw();
    const double pitchDefault = camera.pitch();

    // Baseline expectations: positive yaw (turn right), negative pitch (look down).
    assert(yawDefault > 0.0);
    assert(pitchDefault < 0.0);

    // Reset camera and apply inverted controls.
    camera.SetOrientation(0.0, 0.0);
    camera.SetPosition(0.0, 0.0, 0.0);
    controller.ResetState();

    config.invertFreeLookYaw = true;
    config.invertFreeLookPitch = true;
    controller.SetConfig(config);

    controller.Update(camera, followInput, movement, dt, nullptr);
    const double yawInverted = camera.yaw();
    const double pitchInverted = camera.pitch();

    assert(yawInverted < 0.0);
    assert(pitchInverted > 0.0);

    // Target-lock baseline (non-inverted)
    Camera lockCamera;
    lockCamera.SetOrientation(0.0, 0.0);
    lockCamera.SetPosition(0.0, 0.0, 5.0);

    CameraFollow::CameraFollowState lockState{};
    CameraFollow::CameraFollowInput lockInput{};
    lockInput.isTargetLocked = true;
    lockInput.playerX = 0.0;
    lockInput.playerY = 0.0;
    lockInput.playerZ = 0.0;
    lockInput.mouseLookYawOffset = 0.05;
    lockInput.mouseLookPitchOffset = 0.05;

    CameraFollow::CameraFollowConfig lockConfig = config;
    lockConfig.alwaysTickFreeMode = true;
    lockConfig.transitionSpeed = 500.0;
    lockConfig.posResponsiveness = 500.0;
    lockConfig.rotResponsiveness = 500.0;
    lockConfig.pitchBias = 0.0;
    lockConfig.orbitDistance = 5.0;
    lockConfig.orbitHeight = 0.0;

    // Run target-lock update without inversion.
    UpdateTargetLockCamera(lockCamera, lockState, lockConfig, lockInput, dt, nullptr);
    const double yawLockDefault = lockCamera.yaw();
    const double pitchLockDefault = lockCamera.pitch();
    const double lockedOrbitDefault = lockState.lockedOrbitOffset;

    // Reset camera/state for inverted case.
    lockCamera.SetOrientation(0.0, 0.0);
    lockCamera.SetPosition(0.0, 0.0, 5.0);
    CameraFollow::CameraFollowState lockStateInverted{};
    lockConfig.invertLockYaw = true;
    lockConfig.invertLockPitch = true;

    UpdateTargetLockCamera(lockCamera, lockStateInverted, lockConfig, lockInput, dt, nullptr);
    const double yawLockInverted = lockCamera.yaw();
    const double pitchLockInverted = lockCamera.pitch();
    const double lockedOrbitInverted = lockStateInverted.lockedOrbitOffset;

    assert(yawLockDefault > 0.0);
    assert(pitchLockDefault > 0.0);
    assert(lockedOrbitDefault > 0.0);

    assert(yawLockInverted < yawLockDefault);
    assert(pitchLockInverted < pitchLockDefault);
    assert(lockedOrbitInverted < 0.0);

    std::cout << "Camera invert-axis tests passed" << std::endl;
    return 0;
}
