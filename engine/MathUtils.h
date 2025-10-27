#pragma once

#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Basic vector operations
namespace MathUtils {
    // Calculate yaw and pitch angles to face a target point
    void calculateFacingAngles(const Point3D& currentPos, const Point3D& targetPos, double& yaw, double& pitch) {
        double dx = targetPos.x - currentPos.x;
        double dy = targetPos.y - currentPos.y;
        double dz = targetPos.z - currentPos.z;

        double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
        if (distance > 0.0) {
            yaw = std::atan2(dx, dz) * 180.0 / M_PI;
            pitch = -std::asin(dy / distance) * 180.0 / M_PI;
        }
    }

    // Clamp a value between min and max
    template<typename T>
    T clamp(T value, T min, T max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
}