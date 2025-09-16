#include "Player.h"
#include <iostream>

Player::Player() : x(0.0), y(0.0), vx(1.0) {}
Player::~Player() {}

void Player::Init() {
    x = 0.0;
    y = 0.0;
    vx = 1.0; // 1 unit/sec
    std::cout << "Player initialized at (" << x << ", " << y << ")" << std::endl;
}

void Player::Update(double dt) {
    x += vx * dt;
    // bounce between -5 and 5
    if (x > 5.0) {
        x = 5.0;
        vx = -vx;
    } else if (x < -5.0) {
        x = -5.0;
        vx = -vx;
    }
}

double Player::GetX() const { return x; }
double Player::GetY() const { return y; }
