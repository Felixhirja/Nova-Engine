#pragma once

#include <array>

struct Transform {
    Transform();
    double x, y, z;

    void Translate(double dx, double dy, double dz);
};
