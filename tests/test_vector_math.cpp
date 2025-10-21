#include "../src/CelestialBody.h"

#include <cassert>
#include <cmath>

int main() {
    const double epsilon = 1e-9;

    Vector3 xAxis(1.0, 0.0, 0.0);
    Vector3 yAxis(0.0, 1.0, 0.0);
    Vector3 zAxis = xAxis.Cross(yAxis);
    assert(std::abs(zAxis.x) < epsilon);
    assert(std::abs(zAxis.y) < epsilon);
    assert(std::abs(zAxis.z - 1.0) < epsilon);

    Vector3 reversed = yAxis.Cross(xAxis);
    assert(std::abs(reversed.z + 1.0) < epsilon);

    Vector3 arbitraryA(4.5, -2.0, 10.0);
    Vector3 arbitraryB(-1.5, 3.0, 2.0);
    Vector3 cross = arbitraryA.Cross(arbitraryB);
    double dot = cross.Dot(arbitraryA);
    double dotB = cross.Dot(arbitraryB);
    assert(std::abs(dot) < epsilon);
    assert(std::abs(dotB) < epsilon);

    double expectedDistance = std::sqrt(std::pow(arbitraryA.x - arbitraryB.x, 2) +
                                        std::pow(arbitraryA.y - arbitraryB.y, 2) +
                                        std::pow(arbitraryA.z - arbitraryB.z, 2));
    assert(std::abs(arbitraryA.Distance(arbitraryB) - expectedDistance) < epsilon);
    assert(std::abs(arbitraryA.Distance(arbitraryA)) < epsilon);

    return 0;
}
