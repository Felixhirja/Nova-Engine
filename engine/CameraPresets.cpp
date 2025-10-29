#include "CameraPresets.h"
#include "Camera.h"

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

const std::array<CameraPreset, 3>& GetDefaultCameraPresets() {
    return kDefaultPresets;
}

void ApplyPresetToCamera(Camera& camera, const CameraPreset& preset) {
    camera.SetPosition(preset.x, preset.y, preset.z);
    camera.SetOrientation(preset.pitch, preset.yaw);
    camera.SetZoom(preset.zoom);
    camera.SetTargetZoom(preset.zoom);
}
