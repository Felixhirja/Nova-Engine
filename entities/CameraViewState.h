#pragma once

/**
 * CameraViewState: Camera positioning data for player view
 * Separated into own header to avoid circular dependency issues
 */
struct CameraViewState {
    double worldX = 0.0;
    double worldY = 0.0;
    double worldZ = 0.0;
    double facingYaw = 0.0;
    bool isTargetLocked = false;
};
