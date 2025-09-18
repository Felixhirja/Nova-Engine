#pragma once
#include "Component.h"
#include <limits>
#include <string>

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

struct Acceleration : public Component {
    double ax = 0.0;
    double ay = 0.0;
};

struct Transform2D : public Component {
    double x = 0.0;
    double y = 0.0;
    double rotation = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
};

struct PhysicsBody : public Component {
    double mass = 1.0;
    double drag = 0.0;
    bool affectedByGravity = true;
};

struct Hitbox : public Component {
    double width = 1.0;
    double height = 1.0;
};

struct AnimationState : public Component {
    int currentFrame = 0;
    double frameTimer = 0.0;
    double frameDuration = 0.1;
    bool looping = true;
};

struct Name : public Component {
    std::string value;
};

struct PlayerController : public Component {
    bool moveLeft = false;
    bool moveRight = false;
};

struct MovementBounds : public Component {
    double minX = -std::numeric_limits<double>::infinity();
    double maxX = std::numeric_limits<double>::infinity();
    double minY = -std::numeric_limits<double>::infinity();
    double maxY = std::numeric_limits<double>::infinity();
    bool clampX = false;
    bool clampY = false;
};
