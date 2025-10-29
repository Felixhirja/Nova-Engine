#include "../engine/Camera.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main() {
    Camera c(0.0, 0.0, 1.0, 0.0, Camera::kDefaultYawRadians, 1.0);
    int sx, sy;
    // center should map to center
    c.WorldToScreen(0.0, 0.0, 0.0, 800, 600, sx, sy);
    assert(sx == 400 && sy == 300);

    // one unit to the right should move by the projected scale (clamped by FOV)
    c.SetZoom(10.0);
    c.WorldToScreen(1.0, 0.0, 0.0, 800, 600, sx, sy);
    const double scale = Camera::kDefaultFovDegrees / c.zoom();
    const int expectedOffset = static_cast<int>(scale + 0.5);
    assert(sx == 400 + expectedOffset);

    // move camera center, world point at camera center should map to center
    c.MoveTo(5.0, -3.0, 1.0);
    c.SetZoom(2.0);
    c.WorldToScreen(5.0, -3.0, 1.0, 800, 600, sx, sy);
    assert(sx == 400 && sy == 300);

    // Verify default orientation faces +X (π/2 yaw)
    Camera defaultCamera;
    auto approxEqual = [](double a, double b) {
        return std::abs(a - b) < 1e-6;
    };
    assert(approxEqual(defaultCamera.yaw(), Camera::kDefaultYawRadians));

    const Camera::Basis basis = defaultCamera.BuildBasis();
    assert(approxEqual(basis.forwardX, 1.0));
    assert(approxEqual(basis.forwardY, 0.0));
    assert(approxEqual(basis.forwardZ, 0.0));
    assert(approxEqual(basis.rightX, 0.0));
    assert(approxEqual(basis.rightY, 0.0));
    assert(approxEqual(basis.rightZ, -1.0));
    assert(approxEqual(basis.upX, 0.0));
    assert(approxEqual(basis.upY, 1.0));
    assert(approxEqual(basis.upZ, 0.0));

    // Validate view matrix for π/2 yaw alignment
    Camera viewCamera(4.0, 2.0, -3.0, 0.0, Camera::kDefaultYawRadians, Camera::kDefaultFovDegrees);
    const auto viewMatrix = viewCamera.GetViewMatrix();
    assert(approxEqual(viewMatrix[0], 0.0));   // right.x
    assert(approxEqual(viewMatrix[1], 0.0));   // right.y
    assert(approxEqual(viewMatrix[2], -1.0));  // right.z
    assert(approxEqual(viewMatrix[4], 0.0));   // up.x
    assert(approxEqual(viewMatrix[5], 1.0));   // up.y
    assert(approxEqual(viewMatrix[6], 0.0));   // up.z
    assert(approxEqual(viewMatrix[8], -1.0));  // -forward.x
    assert(approxEqual(viewMatrix[9], 0.0));   // -forward.y
    assert(approxEqual(viewMatrix[10], 0.0));  // -forward.z
    assert(approxEqual(viewMatrix[12], -3.0)); // translation x
    assert(approxEqual(viewMatrix[13], -2.0)); // translation y
    assert(approxEqual(viewMatrix[14], 4.0));  // translation z

    std::cout << "Camera tests passed" << std::endl;
    return 0;
}
