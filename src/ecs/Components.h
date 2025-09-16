#pragma once
#include "Component.h"

struct Position : public Component {
    double x = 0.0;
    double y = 0.0;
};

struct Velocity : public Component {
    double vx = 0.0;
    double vy = 0.0;
};

struct Sprite : public Component {
    int textureHandle = 0;
    int layer = 0;
    int frame = 0;
};
