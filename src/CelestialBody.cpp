#include "CelestialBody.h"
#include <cmath>

// ============================================================================
// Vector3 Implementation
// ============================================================================

double Vector3::Length() const {
    return std::sqrt(x * x + y * y + z * z);
}

Vector3 Vector3::Normalized() const {
    double len = Length();
    if (len < 1e-10) return Vector3(0, 0, 0);
    return Vector3(x / len, y / len, z / len);
}

Vector3 Vector3::operator+(const Vector3& other) const {
    return Vector3(x + other.x, y + other.y, z + other.z);
}

Vector3 Vector3::operator-(const Vector3& other) const {
    return Vector3(x - other.x, y - other.y, z - other.z);
}

Vector3 Vector3::operator*(double scalar) const {
    return Vector3(x * scalar, y * scalar, z * scalar);
}

double Vector3::Dot(const Vector3& other) const {
    return x * other.x + y * other.y + z * other.z;
}
