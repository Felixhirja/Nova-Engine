#include "Camera.h"
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef USE_GLFW
#include <glad/glad.h>
#endif
#include <cmath>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Camera::Camera() : x_(0.0), y_(0.0), z_(0.0), pitch_(0.0), yaw_(0.0), zoom_(1.0), targetZoom_(1.0) {}
Camera::Camera(double x, double y, double z, double pitch, double yaw, double zoom) : x_(x), y_(y), z_(z), pitch_(pitch), yaw_(yaw), zoom_(zoom), targetZoom_(zoom) {}

void Camera::SetPosition(double x, double y, double z) { x_ = x; y_ = y; z_ = z; }
void Camera::SetOrientation(double pitch, double yaw) { pitch_ = pitch; yaw_ = yaw; }
void Camera::MoveTo(double x, double y, double z) { x_ = x; y_ = y; z_ = z; }
void Camera::SetZoom(double z) { zoom_ = z; }

void Camera::LerpTo(double targetX, double targetY, double targetZ, double alpha) {
    // alpha in [0,1], where 1 = instant
    x_ = x_ + (targetX - x_) * alpha;
    y_ = y_ + (targetY - y_) * alpha;
    z_ = z_ + (targetZ - z_) * alpha;
}

void Camera::SetTargetZoom(double z) {
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
    zoom_ = newZoom;
}

void Camera::WorldToScreen(double wx, double wy, double wz, int screenW, int screenH, int &outX, int &outY) const {
    (void)wz; // Unused parameter - simplified 2D projection
    // Simplified 3D to 2D projection approximation: ignore z for now, treat as 2D
    double sx = (wx - x_) * zoom_ + (double)screenW * 0.5;
    double sy = (wy - y_) * zoom_ + (double)screenH * 0.5;
    outX = static_cast<int>(sx + 0.5);
    outY = static_cast<int>(sy + 0.5);
}

#ifdef USE_GLFW
void Camera::ApplyToOpenGL() const {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // Apply zoom as uniform scale
    glScaled(zoom_, zoom_, zoom_);
    
    // Apply camera rotation (pitch and yaw)
    glRotated(-pitch_ * 180.0 / M_PI, 1.0, 0.0, 0.0); // pitch around X axis
    glRotated(-yaw_ * 180.0 / M_PI, 0.0, 1.0, 0.0);   // yaw around Y axis
    
    // Apply camera translation (move world so camera is at origin)
    glTranslated(-x_, -y_, -z_);
}
#else
void Camera::ApplyToOpenGL() const {}
#endif
