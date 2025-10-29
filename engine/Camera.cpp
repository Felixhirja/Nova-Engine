#include "Camera.h"
#ifdef _WIN32
#include <windows.h>
#endif
#if defined(USE_GLFW) || defined(USE_SDL)
#include <glad/glad.h>
#endif
#include <algorithm>
#include <cmath>
#include <array>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Camera::Camera()
    : x_(0.0)
    , y_(0.0)
    , z_(0.0)
    , pitch_(0.0)
    , yaw_(kDefaultYawRadians)
    , zoom_(kDefaultFovDegrees)
    , targetZoom_(kDefaultFovDegrees) {}

Camera::Camera(double x, double y, double z, double pitch, double yaw, double zoom)
    : x_(x)
    , y_(y)
    , z_(z)
    , pitch_(pitch)
    , yaw_(yaw)
    , zoom_(ClampFov(zoom))
    , targetZoom_(ClampFov(zoom)) {}

void Camera::SetPosition(double x, double y, double z) { x_ = x; y_ = y; z_ = z; }
void Camera::SetOrientation(double pitch, double yaw) { pitch_ = pitch; yaw_ = yaw; }
void Camera::MoveTo(double x, double y, double z) { x_ = x; y_ = y; z_ = z; }
void Camera::SetZoom(double z) {
    zoom_ = ClampFov(z);
    targetZoom_ = zoom_;
}

void Camera::LerpTo(double targetX, double targetY, double targetZ, double alpha) {
    // alpha in [0,1], where 1 = instant
    x_ = x_ + (targetX - x_) * alpha;
    y_ = y_ + (targetY - y_) * alpha;
    z_ = z_ + (targetZ - z_) * alpha;
}

void Camera::SetTargetZoom(double z) {
    targetZoom_ = ClampFov(z);
}

void Camera::UpdateZoom(double dt) {
    // smooth interpolation towards targetZoom_: simple exponential lerp
    if (dt <= 0.0) return;
    double speed = 6.0; // larger -> faster
    // guard against very large dt
    if (speed * dt > 50.0) dt = 50.0 / speed;
    double alpha = 1.0 - std::exp(-speed * dt);
    targetZoom_ = ClampFov(targetZoom_);
    double newZoom = zoom_ + (targetZoom_ - zoom_) * alpha;
    if (!std::isfinite(newZoom)) return;
    zoom_ = ClampFov(newZoom);
}

void Camera::WorldToScreen(double wx, double wy, double wz, int screenW, int screenH, int &outX, int &outY) const {
    (void)wz; // Unused parameter - simplified 2D projection
    // Simplified 3D to 2D projection approximation: ignore z for now, treat as 2D
    const double scale = (zoom_ > 0.0) ? (kDefaultFovDegrees / zoom_) : 1.0;
    double sx = (wx - x_) * scale + (double)screenW * 0.5;
    double sy = (wy - y_) * scale + (double)screenH * 0.5;
    outX = static_cast<int>(sx + 0.5);
    outY = static_cast<int>(sy + 0.5);
}

#if defined(USE_GLFW) || defined(USE_SDL)
void Camera::ApplyToOpenGL() const {
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    const double pitchDegrees = -pitch_ * 180.0 / M_PI;
    const double yawDegrees   = -yaw_   * 180.0 / M_PI;

    // Apply camera rotation (pitch, then yaw) followed by translation to camera space
    glRotated(pitchDegrees, 1.0, 0.0, 0.0);
    glRotated(yawDegrees,   0.0, 1.0, 0.0);
    glTranslated(-x_, -y_, -z_);
}
#else
void Camera::ApplyToOpenGL() const {}
#endif

double Camera::ClampFov(double fov) noexcept {
    if (!std::isfinite(fov) || fov <= 0.0) {
        return kDefaultFovDegrees;
    }
    return std::clamp(fov, kMinFovDegrees, kMaxFovDegrees);
}

Camera::Basis Camera::BuildBasis(bool includePitchInForward) const noexcept {
    const double cy = std::cos(yaw_);
    const double sy = std::sin(yaw_);
    const double cp = std::cos(pitch_);
    const double sp = std::sin(pitch_);

    // Forward vector in world space (engine convention: +X right, +Y up, +Z forward)
    const double horizScale = includePitchInForward ? cp : 1.0;
    double forwardX = sy * horizScale;
    double forwardY = includePitchInForward ? sp : 0.0;
    double forwardZ = cy * horizScale;

    // Normalise forward; guard against degeneracy near poles.
    auto normalise = [](double& x, double& y, double& z) {
        const double lenSq = x * x + y * y + z * z;
        if (lenSq < 1e-12) {
            x = 0.0;
            y = 0.0;
            z = -1.0;
            return;
        }
        const double invLen = 1.0 / std::sqrt(lenSq);
        x *= invLen;
        y *= invLen;
        z *= invLen;
    };

    normalise(forwardX, forwardY, forwardZ);

    // Reference up axis (world up is +Y)
    const double worldUpX = 0.0;
    const double worldUpY = 1.0;
    const double worldUpZ = 0.0;

    // Derive right from forward and world up. If we are near parallel to up, fall back.
    double rightX = worldUpY * forwardZ - worldUpZ * forwardY;
    double rightY = worldUpZ * forwardX - worldUpX * forwardZ;
    double rightZ = worldUpX * forwardY - worldUpY * forwardX;

    double rightLenSq = rightX * rightX + rightY * rightY + rightZ * rightZ;
    if (rightLenSq < 1e-12) {
        // Forward almost matches the up axis; choose an arbitrary orthogonal axis.
        rightX = 1.0;
        rightY = 0.0;
        rightZ = 0.0;
        rightLenSq = 1.0;
    }
    const double invRightLen = 1.0 / std::sqrt(rightLenSq);
    rightX *= invRightLen;
    rightY *= invRightLen;
    rightZ *= invRightLen;

    // Recompute a precise up via right × forward to keep the basis orthonormal.
    double upX = forwardY * rightZ - forwardZ * rightY;
    double upY = forwardZ * rightX - forwardX * rightZ;
    double upZ = forwardX * rightY - forwardY * rightX;
    normalise(upX, upY, upZ);

    return Basis{forwardX, forwardY, forwardZ,
                 rightX, rightY, rightZ,
                 upX, upY, upZ};
}

std::array<double, 16> Camera::GetViewMatrix() const noexcept {
    const Basis basis = BuildBasis(true);

    const double transX = -(basis.rightX * x_ + basis.rightY * y_ + basis.rightZ * z_);
    const double transY = -(basis.upX * x_ + basis.upY * y_ + basis.upZ * z_);
    const double transZ =  (basis.forwardX * x_ + basis.forwardY * y_ + basis.forwardZ * z_);

    return {basis.rightX, basis.rightY, basis.rightZ, 0.0,
            basis.upX,    basis.upY,    basis.upZ,    0.0,
            -basis.forwardX, -basis.forwardY, -basis.forwardZ, 0.0,
            transX,       transY,       transZ,       1.0};
}

std::array<double, 16> Camera::GetProjectionMatrix(double aspectRatio,
                                                   double nearPlaneMeters,
                                                   double farPlaneMeters) const noexcept {
    const double safeAspect = (aspectRatio > 0.0 && std::isfinite(aspectRatio)) ? aspectRatio : 1.0;
    const double safeNear = std::max(1e-3, std::isfinite(nearPlaneMeters) ? nearPlaneMeters : 0.1);
    const double safeFar = std::max(safeNear + 1e-3,
                                    std::isfinite(farPlaneMeters) ? farPlaneMeters : safeNear + 1000.0);

    const double fovDegrees = ClampFov(zoom_);
    const double fovRadians = fovDegrees * (M_PI / 180.0);
    const double f = 1.0 / std::tan(fovRadians * 0.5);
    const double invDepth = 1.0 / (safeNear - safeFar);

    std::array<double, 16> proj{};
    proj[0] = f / safeAspect;
    proj[5] = f;
    proj[10] = (safeFar + safeNear) * invDepth;
    proj[11] = -1.0;
    proj[14] = (2.0 * safeFar * safeNear) * invDepth;
    return proj;
}
