#include "../engine/Camera.h"
#include "../engine/CameraPresets.h"

#include <iostream>
#include <cmath>

namespace {
bool approxEqual(double a, double b, double tol = 1e-6) {
    return std::abs(a - b) <= tol;
}
}

int main() {
    Camera camera;
    const auto& presets = GetDefaultCameraPresets();

    for (size_t i = 0; i < presets.size(); ++i) {
        ApplyPresetToCamera(camera, presets[i]);
        if (!approxEqual(camera.x(), presets[i].x) ||
            !approxEqual(camera.y(), presets[i].y) ||
            !approxEqual(camera.z(), presets[i].z) ||
            !approxEqual(camera.pitch(), presets[i].pitch) ||
            !approxEqual(camera.yaw(), presets[i].yaw) ||
            !approxEqual(camera.zoom(), presets[i].zoom) ||
            !approxEqual(camera.targetZoom(), presets[i].zoom)) {
            std::cerr << "Preset " << (i + 1) << " failed to apply correctly" << std::endl;
            return static_cast<int>(i + 1);
        }
    }

    std::cout << "Camera preset tests passed" << std::endl;
    return 0;
}
