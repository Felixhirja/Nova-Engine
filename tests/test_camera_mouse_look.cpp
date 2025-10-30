#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <cmath>

#include "engine/CameraSystem.h"
#include "engine/Simulation.h"
#include "engine/EntityManager.h"

// Test fixture for camera mouse look functionality
class CameraMouseLookTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create camera with default position and orientation
        camera = std::make_unique<Camera>(0.0f, 0.0f, 10.0f, 0.0f, 0.0f, 60.0f);

        // Create entity manager and simulation
        entityManager = std::make_unique<EntityManager>();
        simulation = std::make_unique<Simulation>();

        // Initialize camera follow controller with default config
        config = CameraFollow::CameraFollowConfig{};
        controller.SetConfig(config);
        controller.ResetState();
    }

    void TearDown() override {
        camera.reset();
        entityManager.reset();
        simulation.reset();
    }

    std::unique_ptr<Camera> camera;
    std::unique_ptr<EntityManager> entityManager;
    std::unique_ptr<Simulation> simulation;
    CameraFollow::CameraFollowConfig config;
    CameraFollowController controller;
};

// Test mouse look in free camera mode
TEST_F(CameraMouseLookTest, FreeCameraMouseLook) {
    // Initial camera orientation
    const double initialYaw = camera->yaw();
    const double initialPitch = camera->pitch();

    // Simulate mouse movement (100 pixels right, 50 pixels down)
    CameraMovementInput movementInput;
    movementInput.mouseDeltaX = 100.0;
    movementInput.mouseDeltaY = 50.0;

    // Create follow input (not target locked)
    CameraFollow::CameraFollowInput followInput;
    followInput.isTargetLocked = false;
    followInput.playerX = 0.0;
    followInput.playerY = 0.0;
    followInput.playerZ = 0.0;

    // Update camera (simulate one frame at 60 FPS)
    const double deltaTime = 1.0 / 60.0;
    controller.Update(*camera, followInput, movementInput, deltaTime);

    // Check that camera orientation changed
    const double newYaw = camera->yaw();
    const double newPitch = camera->pitch();

    // Yaw should increase (mouse moved right)
    EXPECT_GT(newYaw, initialYaw);

    // Pitch should increase (mouse moved down, but note: Y is inverted in typical mouse controls)
    // The actual direction depends on invert settings, but there should be some change
    EXPECT_NE(newPitch, initialPitch);

    // Calculate expected change based on sensitivity
    const double expectedYawDelta = 100.0 * config.freeLookSensYaw;
    const double expectedPitchDelta = -50.0 * config.freeLookSensPitch; // Y inverted

    EXPECT_NEAR(newYaw, initialYaw + expectedYawDelta, 0.01);
    EXPECT_NEAR(newPitch, initialPitch + expectedPitchDelta, 0.01);
}

// Test that mouse look is disabled in target lock mode
TEST_F(CameraMouseLookTest, TargetLockDisablesFreeLook) {
    // Initial camera orientation
    const double initialYaw = camera->yaw();
    const double initialPitch = camera->pitch();

    // Simulate mouse movement
    CameraMovementInput movementInput;
    movementInput.mouseDeltaX = 100.0;
    movementInput.mouseDeltaY = 50.0;

    // Create follow input (target locked)
    CameraFollow::CameraFollowInput followInput;
    followInput.isTargetLocked = true;
    followInput.playerX = 0.0;
    followInput.playerY = 0.0;
    followInput.playerZ = 0.0;

    // Update camera
    const double deltaTime = 1.0 / 60.0;
    controller.Update(*camera, followInput, movementInput, deltaTime);

    // Camera should not change orientation due to mouse look in target lock mode
    // (target lock uses different mouse handling)
    const double newYaw = camera->yaw();
    const double newPitch = camera->pitch();

    // Orientation should remain the same (target lock handles mouse differently)
    EXPECT_EQ(newYaw, initialYaw);
    EXPECT_EQ(newPitch, initialPitch);
}

// Test mouse look sensitivity
TEST_F(CameraMouseLookTest, MouseLookSensitivity) {
    // Test with custom sensitivity
    auto customConfig = config;
    customConfig.freeLookSensYaw = 0.005;   // Double default
    customConfig.freeLookSensPitch = 0.004; // Double default
    controller.SetConfig(customConfig);

    const double initialYaw = camera->yaw();
    const double initialPitch = camera->pitch();

    // Small mouse movement
    CameraMovementInput movementInput;
    movementInput.mouseDeltaX = 10.0;
    movementInput.mouseDeltaY = 10.0;

    CameraFollow::CameraFollowInput followInput;
    followInput.isTargetLocked = false;
    followInput.playerX = 0.0;
    followInput.playerY = 0.0;
    followInput.playerZ = 0.0;

    const double deltaTime = 1.0 / 60.0;
    controller.Update(*camera, followInput, movementInput, deltaTime);

    const double newYaw = camera->yaw();
    const double newPitch = camera->pitch();

    // Check sensitivity is applied correctly
    const double expectedYawDelta = 10.0 * customConfig.freeLookSensYaw;
    const double expectedPitchDelta = -10.0 * customConfig.freeLookSensPitch;

    EXPECT_NEAR(newYaw, initialYaw + expectedYawDelta, 0.001);
    EXPECT_NEAR(newPitch, initialPitch + expectedPitchDelta, 0.001);
}

// Test mouse look deadzone
TEST_F(CameraMouseLookTest, MouseLookDeadzone) {
    const double initialYaw = camera->yaw();
    const double initialPitch = camera->pitch();

    // Mouse movement below deadzone threshold
    CameraMovementInput movementInput;
    movementInput.mouseDeltaX = 0.1;  // Below deadzone of 0.2
    movementInput.mouseDeltaY = 0.1;

    CameraFollow::CameraFollowInput followInput;
    followInput.isTargetLocked = false;
    followInput.playerX = 0.0;
    followInput.playerY = 0.0;
    followInput.playerZ = 0.0;

    const double deltaTime = 1.0 / 60.0;
    controller.Update(*camera, followInput, movementInput, deltaTime);

    const double newYaw = camera->yaw();
    const double newPitch = camera->pitch();

    // Camera should not move due to deadzone
    EXPECT_EQ(newYaw, initialYaw);
    EXPECT_EQ(newPitch, initialPitch);
}

// Test transition from target lock to free camera enables mouse look immediately
TEST_F(CameraMouseLookTest, InstantTransitionToFreeCamera) {
    // Start in target lock mode
    CameraFollow::CameraFollowInput followInput;
    followInput.isTargetLocked = true;
    followInput.playerX = 0.0;
    followInput.playerY = 0.0;
    followInput.playerZ = 0.0;

    // Update to establish target lock state
    CameraMovementInput movementInput;
    movementInput.mouseDeltaX = 0.0;
    movementInput.mouseDeltaY = 0.0;

    const double deltaTime = 1.0 / 60.0;
    controller.Update(*camera, followInput, movementInput, deltaTime);

    // Now switch to free camera mode
    followInput.isTargetLocked = false;
    movementInput.mouseDeltaX = 50.0;  // Apply mouse movement
    movementInput.mouseDeltaY = 25.0;

    const double initialYaw = camera->yaw();
    const double initialPitch = camera->pitch();

    // Update - mouse look should work immediately
    controller.Update(*camera, followInput, movementInput, deltaTime);

    const double newYaw = camera->yaw();
    const double newPitch = camera->pitch();

    // Camera should respond to mouse movement immediately
    EXPECT_NE(newYaw, initialYaw);
    EXPECT_NE(newPitch, initialPitch);
}