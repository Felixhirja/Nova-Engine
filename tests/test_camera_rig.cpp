#include "../engine/CameraSystem.h"

#include <cassert>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
constexpr double kEpsilon = 1e-6;

bool nearlyEqual(double a, double b, double epsilon = kEpsilon) {
    return std::abs(a - b) <= epsilon;
}

bool isOrthonormal(const Camera::Basis& basis) {
    const double fLen = std::sqrt(basis.forwardX * basis.forwardX +
                                  basis.forwardY * basis.forwardY +
                                  basis.forwardZ * basis.forwardZ);
    const double rLen = std::sqrt(basis.rightX * basis.rightX +
                                  basis.rightY * basis.rightY +
                                  basis.rightZ * basis.rightZ);
    const double uLen = std::sqrt(basis.upX * basis.upX +
                                  basis.upY * basis.upY +
                                  basis.upZ * basis.upZ);
    if (!nearlyEqual(fLen, 1.0) || !nearlyEqual(rLen, 1.0) || !nearlyEqual(uLen, 1.0)) {
        return false;
    }

    const double frDot = basis.forwardX * basis.rightX +
                         basis.forwardY * basis.rightY +
                         basis.forwardZ * basis.rightZ;
    const double fuDot = basis.forwardX * basis.upX +
                         basis.forwardY * basis.upY +
                         basis.forwardZ * basis.upZ;
    const double ruDot = basis.rightX * basis.upX +
                         basis.rightY * basis.upY +
                         basis.rightZ * basis.upZ;
    return nearlyEqual(frDot, 0.0) && nearlyEqual(fuDot, 0.0) && nearlyEqual(ruDot, 0.0);
}
}

int main() {
    {
        Camera camera(0.0, 0.0, 0.0, 0.0, 0.0, Camera::kDefaultFovDegrees);
        Camera::Basis basis = camera.BuildBasis(true);
        assert(nearlyEqual(basis.forwardX, 0.0));
        assert(nearlyEqual(basis.forwardY, 0.0));
        assert(nearlyEqual(basis.forwardZ, 1.0));
        assert(nearlyEqual(basis.rightX, 1.0));
        assert(nearlyEqual(basis.rightY, 0.0));
        assert(nearlyEqual(basis.rightZ, 0.0));
        assert(nearlyEqual(basis.upX, 0.0));
        assert(nearlyEqual(basis.upY, 1.0));
        assert(nearlyEqual(basis.upZ, 0.0));
        assert(isOrthonormal(basis));
    }

    {
        const double yaw = M_PI * 0.5; // 90° yaw
        Camera camera(0.0, 0.0, 0.0, 0.0, yaw, Camera::kDefaultFovDegrees);
        Camera::Basis basis = camera.BuildBasis(true);
        assert(nearlyEqual(basis.forwardX, 1.0));
        assert(nearlyEqual(basis.forwardY, 0.0));
        assert(nearlyEqual(basis.forwardZ, 0.0));
        assert(nearlyEqual(basis.rightX, 0.0));
        assert(nearlyEqual(basis.rightY, 0.0));
        assert(nearlyEqual(basis.rightZ, -1.0));
        assert(isOrthonormal(basis));
    }

    {
        const double pitch = Camera::kDefaultFovDegrees * (M_PI / 180.0) * 0.25; // arbitrary tilt
        Camera camera(0.0, 0.0, 0.0, pitch, 0.0, Camera::kDefaultFovDegrees);
        Camera::Basis basis = camera.BuildBasis(true);
        assert(basis.forwardY > 0.0);
        assert(isOrthonormal(basis));
    }

    {
        const double yaw = M_PI * 0.25;
        Camera camera(0.0, 0.0, 0.0, 0.2, yaw, Camera::kDefaultFovDegrees);
        Camera::Basis basis = camera.BuildBasis(false);
        // When ignoring pitch the forward vector should stay on XZ plane.
        assert(nearlyEqual(basis.forwardY, 0.0));
        assert(isOrthonormal(basis));
    }

    std::cout << "Camera rig transform tests passed" << std::endl;
    return 0;
}
