#pragma once

#include <array>

class Camera;

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
