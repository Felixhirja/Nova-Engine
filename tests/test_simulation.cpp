#include "../src/Simulation.h"
#include <iostream>
#include <cmath>

int main() {
    Simulation sim;
    sim.Init();

    // Test 1: with right input for 1 second, expect x ~ 2.0 (0.5 * a * t^2, a=4)
    sim.SetPlayerInput(false, false, false, false, false, true, 0.0);
    const double dt = 1.0 / 60.0;
    for (int i = 0; i < 60; ++i) sim.Update(dt);
    double x = sim.GetPlayerX();
    std::cout << "Test1 player x=" << x << std::endl;
    if (std::abs(x - 3.5) > 0.2) {
        std::cerr << "Test1 FAILED: expected ~3.5" << std::endl;
        return 1;
    }

    // Test 2: hold right for long time, position should clamp at <= 5.0
    for (int i = 0; i < 600; ++i) sim.Update(dt);
    x = sim.GetPlayerX();
    std::cout << "Test2 player x=" << x << std::endl;
    if (x > 5.0 + 1e-6) {
        std::cerr << "Test2 FAILED: expected <=5.0" << std::endl;
        return 2;
    }

    // Reset simulation for Y-axis tests
    sim.Init();

    // Test 3: with forward input for 1 second, expect y ~ 3.5
    sim.SetPlayerInput(true, false, false, false, false, false, 0.0);
    for (int i = 0; i < 60; ++i) sim.Update(dt);
    double y = sim.GetPlayerY();
    x = sim.GetPlayerX();
    std::cout << "Test3 player y=" << y << " x=" << x << std::endl;
    if (std::abs(y - 3.5) > 0.2) {
        std::cerr << "Test3 FAILED: expected y ~3.5" << std::endl;
        return 3;
    }
    if (std::abs(x) > 0.2) {
        std::cerr << "Test3 FAILED: expected x near 0" << std::endl;
        return 4;
    }

    // Test 4: hold forward for long time, position should clamp at <= 5.0 on Y
    for (int i = 0; i < 600; ++i) sim.Update(dt);
    y = sim.GetPlayerY();
    std::cout << "Test4 player y=" << y << std::endl;
    if (y > 5.0 + 1e-6) {
        std::cerr << "Test4 FAILED: expected y <=5.0" << std::endl;
        return 5;
    }

    // Reset simulation to test backward movement clamping
    sim.Init();
    sim.SetPlayerInput(false, true, false, false, false, false, 0.0);
    for (int i = 0; i < 600; ++i) sim.Update(dt);
    y = sim.GetPlayerY();
    std::cout << "Test5 player y=" << y << std::endl;
    if (y < -5.0 - 1e-6) {
        std::cerr << "Test5 FAILED: expected y >= -5.0" << std::endl;
        return 6;
    }

    // Test 6: diagonal forward-right input for 1 second, expect similar displacement on X and Y
    sim.Init();
    sim.SetPlayerInput(true, false, false, false, false, true, 0.0);
    for (int i = 0; i < 60; ++i) sim.Update(dt);
    double diagX = sim.GetPlayerX();
    double diagY = sim.GetPlayerY();
    std::cout << "Test6 player x=" << diagX << " y=" << diagY << std::endl;
    if (std::abs(diagX - 3.5) > 0.3) {
        std::cerr << "Test6 FAILED: expected x ~3.5" << std::endl;
        return 7;
    }
    if (std::abs(diagY - 3.5) > 0.3) {
        std::cerr << "Test6 FAILED: expected y ~3.5" << std::endl;
        return 8;
    }
    if (std::abs(diagX - diagY) > 0.2) {
        std::cerr << "Test6 FAILED: expected x and y to match for diagonal" << std::endl;
        return 9;
    }

    // Test 7: zero-gravity style scenario (no bounds) should allow drifting beyond limits
    Simulation zeroSim;
    EntityManager zeroEm;
    zeroSim.Init(&zeroEm);
    if (auto* zeroBounds = zeroEm.GetComponent<MovementBounds>(zeroSim.GetPlayerEntity())) {
        zeroBounds->clampX = false;
        zeroBounds->clampY = false;
        zeroBounds->clampZ = false;
    }
    if (auto* zeroPhysics = zeroEm.GetComponent<PlayerPhysics>(zeroSim.GetPlayerEntity())) {
        zeroPhysics->enableGravity = false;
        zeroPhysics->thrustMode = false;
    }
    zeroSim.SetPlayerInput(true, false, false, false, false, false, 0.0);
    for (int i = 0; i < 600; ++i) zeroSim.Update(dt);
    double zeroX = zeroSim.GetPlayerX();
    double zeroY = zeroSim.GetPlayerY();
    std::cout << "Test7 zero-gravity x=" << zeroX << " y=" << zeroY << std::endl;
    if (zeroY <= 5.0 + 1e-6) {
        std::cerr << "Test7 FAILED: expected y to exceed clamp in zero-gravity" << std::endl;
        return 10;
    }
    if (std::abs(zeroX) > 0.3) {
        std::cerr << "Test7 FAILED: expected minimal X drift when only moving forward" << std::endl;
        return 11;
    }

    // Test 8: jump should lift player off ground and land back with gravity
    sim.Init();
    sim.SetUseThrustMode(false);
    sim.SetPlayerInput(false, false, false, false, false, false, 0.0);
    for (int i = 0; i < 5; ++i) sim.Update(dt);
    sim.SetPlayerInput(false, false, true, false, false, false, 0.0);
    sim.Update(dt);
    double jumpZ = sim.GetPlayerZ();
    std::cout << "Test8 jump z=" << jumpZ << std::endl;
    if (jumpZ <= 0.05) {
        std::cerr << "Test8 FAILED: expected jump to move player upward" << std::endl;
        return 12;
    }
    sim.SetPlayerInput(false, false, false, false, false, false, 0.0);
    for (int i = 0; i < 240; ++i) sim.Update(dt);
    double landZ = sim.GetPlayerZ();
    if (std::abs(landZ) > 0.05) {
        std::cerr << "Test8 FAILED: expected player to land back on ground" << std::endl;
        return 13;
    }

    // Test 9: thrust mode allows sustained hover while button held
    sim.Init();
    sim.SetUseThrustMode(true);
    sim.SetPlayerInput(false, false, true, false, false, false, 0.0);
    for (int i = 0; i < 120; ++i) sim.Update(dt);
    double thrustZ = sim.GetPlayerZ();
    std::cout << "Test9 thrust z=" << thrustZ << std::endl;
    if (thrustZ <= 1.5) {
        std::cerr << "Test9 FAILED: expected thrust to gain altitude" << std::endl;
        return 14;
    }
    sim.SetPlayerInput(false, false, false, false, false, false, 0.0);
    for (int i = 0; i < 240; ++i) sim.Update(dt);
    double thrustLandZ = sim.GetPlayerZ();
    if (thrustLandZ > 0.1) {
        std::cerr << "Test9 FAILED: expected thrust mode to settle back to ground" << std::endl;
        return 15;
    }

    // Test 10: configurable movement parameters should adjust acceleration and persist across Init()
    sim.Init();
    MovementParameters customParams;
    customParams.strafeAcceleration = 2.0;
    customParams.strafeDeceleration = 1.0;
    customParams.strafeMaxSpeed = 2.0;
    customParams.forwardAcceleration = 1.5;
    customParams.backwardAcceleration = 1.5;
    customParams.forwardDeceleration = 1.5;
    customParams.backwardDeceleration = 1.5;
    customParams.forwardMaxSpeed = 2.5;
    customParams.backwardMaxSpeed = 2.5;
    customParams.friction = 0.0;

    sim.ConfigureMovementParameters(customParams);
    const MovementParameters& appliedParams = sim.GetMovementParameters();
    if (std::abs(appliedParams.strafeAcceleration - customParams.strafeAcceleration) > 1e-6) {
        std::cerr << "Test10 FAILED: simulation did not store custom movement parameters" << std::endl;
        return 16;
    }

    sim.SetPlayerInput(false, false, false, false, false, true, 0.0);
    for (int i = 0; i < 60; ++i) sim.Update(dt);
    double tunedX = sim.GetPlayerX();
    std::cout << "Test10 tuned x=" << tunedX << std::endl;
    if (tunedX > 2.0 || tunedX < 0.5) {
        std::cerr << "Test10 FAILED: expected tuned movement to be slower than default" << std::endl;
        return 17;
    }

    // Reinitialize to ensure parameters persist across Init()
    sim.Init();
    if (std::abs(sim.GetMovementParameters().strafeAcceleration - customParams.strafeAcceleration) > 1e-6) {
        std::cerr << "Test10 FAILED: custom parameters did not persist after Init" << std::endl;
        return 18;
    }
    sim.SetPlayerInput(false, false, false, false, false, true, 0.0);
    for (int i = 0; i < 60; ++i) sim.Update(dt);
    double tunedXAfterInit = sim.GetPlayerX();
    std::cout << "Test10 tuned reinit x=" << tunedXAfterInit << std::endl;
    if (std::abs(tunedXAfterInit - tunedX) > 0.3) {
        std::cerr << "Test10 FAILED: expected consistent behavior after Init" << std::endl;
        return 19;
    }

    // Test 11: movement bounds can be customized via configuration API
    sim.Init();
    MovementBounds customBounds = sim.GetMovementBounds();
    customBounds.maxX = 10.0;
    customBounds.maxY = 10.0;
    customBounds.minY = -10.0;
    sim.ConfigureMovementBounds(customBounds);
    sim.SetPlayerInput(false, false, false, false, false, true, 0.0);
    for (int i = 0; i < 600; ++i) sim.Update(dt);
    double wideX = sim.GetPlayerX();
    std::cout << "Test11 widened bounds x=" << wideX << std::endl;
    if (wideX < 9.5) {
        std::cerr << "Test11 FAILED: expected movement to reach wider bound" << std::endl;
        return 20;
    }
    if (wideX > 10.0 + 1e-6) {
        std::cerr << "Test11 FAILED: expected clamp at new max X" << std::endl;
        return 21;
    }

    sim.SetPlayerInput(true, false, false, false, false, false, 0.0);
    for (int i = 0; i < 600; ++i) sim.Update(dt);
    double wideY = sim.GetPlayerY();
    std::cout << "Test11 widened bounds y=" << wideY << std::endl;
    if (wideY > 10.0 + 1e-6 || wideY < -10.0 - 1e-6) {
        std::cerr << "Test11 FAILED: expected clamp within customized Y bounds" << std::endl;
        return 22;
    }

    sim.Init();
    if (std::abs(sim.GetMovementBounds().maxX - customBounds.maxX) > 1e-6) {
        std::cerr << "Test11 FAILED: custom bounds did not persist after Init" << std::endl;
        return 23;
    }

    // Test 12: movement parameters can be loaded from configuration profiles
    sim.SetMovementParametersConfigPath("assets/config/player_movement.ini");
    sim.SetMovementParametersProfile("slow");
    sim.Init();
    const MovementParameters& slowProfile = sim.GetMovementParameters();
    std::cout << "Test12 slow profile accel=" << slowProfile.strafeAcceleration << " friction=" << slowProfile.friction << std::endl;
    if (std::abs(slowProfile.strafeAcceleration - 2.0) > 1e-6 || std::abs(slowProfile.friction - 0.25) > 1e-6) {
        std::cerr << "Test12 FAILED: slow profile not applied from config" << std::endl;
        return 24;
    }

    sim.SetMovementParametersProfile("fast");
    sim.Init();
    const MovementParameters& fastProfile = sim.GetMovementParameters();
    std::cout << "Test12 fast profile accel=" << fastProfile.strafeAcceleration << " maxSpeed=" << fastProfile.strafeMaxSpeed << std::endl;
    if (std::abs(fastProfile.strafeAcceleration - 8.0) > 1e-6 || std::abs(fastProfile.strafeMaxSpeed - 10.0) > 1e-6) {
        std::cerr << "Test12 FAILED: fast profile not applied from config" << std::endl;
        return 25;
    }

    MovementParameters manualParams = fastProfile;
    manualParams.strafeAcceleration = 3.3;
    manualParams.friction = 0.05;
    sim.ConfigureMovementParameters(manualParams);
    sim.Init();
    const MovementParameters& manualApplied = sim.GetMovementParameters();
    std::cout << "Test12 manual profile accel=" << manualApplied.strafeAcceleration << " friction=" << manualApplied.friction << std::endl;
    if (std::abs(manualApplied.strafeAcceleration - manualParams.strafeAcceleration) > 1e-6 ||
        std::abs(manualApplied.friction - manualParams.friction) > 1e-6) {
        std::cerr << "Test12 FAILED: manual override should persist when config disabled" << std::endl;
        return 26;
    }

    std::cout << "All tests passed." << std::endl;
    return 0;
}
