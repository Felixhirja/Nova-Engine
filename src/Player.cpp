#include "Player.h"
#include <iostream>

Player::Player() : x(0.0), y(0.0), vx(1.0) {}
Player::~Player() {}

void Player::Init() {
    x = 0.0;
    y = 0.0;
    vx = 0.0; // start stationary
    ax = 0.0;
    std::cout << "Player initialized at (" << x << ", " << y << ")" << std::endl;
}

void Player::Update(double dt, bool inputLeft, bool inputRight) {
    const double accel = 4.0; // units per second^2
    const double maxSpeed = 5.0;
    // set acceleration from input
    ax = 0.0;
    if (inputLeft) ax -= accel;
    if (inputRight) ax += accel;

    vx += ax * dt;
    // clamp
    if (vx > maxSpeed) vx = maxSpeed;
    if (vx < -maxSpeed) vx = -maxSpeed;

    x += vx * dt;

    // simple world bounds
    if (x > 5.0) { x = 5.0; vx = 0.0; }
    if (x < -5.0) { x = -5.0; vx = 0.0; }
}

double Player::GetX() const { return x; }
double Player::GetY() const { return y; }
