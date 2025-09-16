#include "Camera.h"
#include <cmath>

Camera::Camera() : x_(0.0), y_(0.0), zoom_(1.0) {}
Camera::Camera(double x, double y, double zoom) : x_(x), y_(y), zoom_(zoom) {}

void Camera::SetPosition(double x, double y) { x_ = x; y_ = y; }
void Camera::MoveTo(double x, double y) { x_ = x; y_ = y; }
void Camera::SetZoom(double z) { zoom_ = z; }

void Camera::LerpTo(double targetX, double targetY, double alpha) {
    // alpha in [0,1], where 1 = instant
    x_ = x_ + (targetX - x_) * alpha;
    y_ = y_ + (targetY - y_) * alpha;
}

void Camera::SetTargetZoom(double z) {
    if (z < 0.0001) z = 0.0001;
    targetZoom_ = z;
}

void Camera::UpdateZoom(double dt) {
    // smooth interpolation towards targetZoom_: simple exponential lerp
    if (dt <= 0.0) return;
    double speed = 6.0; // larger -> faster
    // guard against very large dt
    if (speed * dt > 50.0) dt = 50.0 / speed;
    double alpha = 1.0 - std::exp(-speed * dt);
    double newZoom = zoom_ + (targetZoom_ - zoom_) * alpha;
    if (!std::isfinite(newZoom)) return;
    // clamp zoom to reasonable range
    if (newZoom < 0.0001) newZoom = 0.0001;
    if (newZoom > 10000.0) newZoom = 10000.0;
    zoom_ = newZoom;
}

void Camera::WorldToScreen(double wx, double wy, int screenW, int screenH, int &outX, int &outY) const {
    // Simple orthographic center camera: world units map 1:1 scaled by zoom to pixels
    // Center of screen corresponds to camera position
    double sx = (wx - x_) * zoom_ + (double)screenW * 0.5;
    double sy = (wy - y_) * zoom_ + (double)screenH * 0.5;
    outX = static_cast<int>(sx + 0.5);
    outY = static_cast<int>(sy + 0.5);
}
