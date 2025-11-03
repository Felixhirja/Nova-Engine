#pragma once

#include <cstring>

// Test components used in memory optimization tests
// These are separate from production components to avoid dependencies

struct SimplePosition {
    double x = 0.0, y = 0.0, z = 0.0;
};

struct SimpleVelocity {
    double vx = 0.0, vy = 0.0, vz = 0.0;
};

struct SimpleTestComponent {
    char data[1024]; // 1KB component for testing memory usage
    SimpleTestComponent() { std::memset(data, 0, sizeof(data)); }
};
