#include "Transform.h"

Transform::Transform() : x(0.0), y(0.0), z(0.0) {}

void Transform::Translate(double dx, double dy, double dz) {
    x += dx;
    y += dy;
    z += dz;
}
