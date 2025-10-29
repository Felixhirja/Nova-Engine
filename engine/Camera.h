#pragma once

#include <array>

class Camera {
public:
    static constexpr double kMinFovDegrees = 30.0;
    static constexpr double kMaxFovDegrees = 90.0;
    static constexpr double kDefaultFovDegrees = 60.0;

    struct Basis {
        double forwardX;
        double forwardY;
        double forwardZ;
        double rightX;
        double rightY;
        double rightZ;
        double upX;
        double upY;
        double upZ;
    };

    Camera();
    Camera(double x, double y, double z, double pitch, double yaw, double zoom);

    void SetPosition(double x, double y, double z);
    void SetOrientation(double pitch, double yaw);
    void SetZoom(double z);

    // Smoothly move camera towards target over time (lerp)
    void LerpTo(double targetX, double targetY, double targetZ, double alpha);
    // Instant move
    void MoveTo(double x, double y, double z);

    // Convert world coords to screen coords (for 3D, simplified orthographic approximation)
    void WorldToScreen(double wx, double wy, double wz, int screenW, int screenH, int &outX, int &outY) const;

    // Apply camera transformation to OpenGL modelview matrix
    void ApplyToOpenGL() const;

    // Column-major view matrix (right, up, -forward, translation).
    std::array<double, 16> GetViewMatrix() const noexcept;

    // Column-major perspective projection using vertical FOV (degrees).
    std::array<double, 16> GetProjectionMatrix(double aspectRatio,
                                               double nearPlaneMeters,
                                               double farPlaneMeters) const noexcept;

    double x() const { return x_; }
    double y() const { return y_; }
    double z() const { return z_; }
    double pitch() const { return pitch_; }
    double yaw() const { return yaw_; }
    double zoom() const { return zoom_; }
    double targetZoom() const { return targetZoom_; }
    void SetTargetZoom(double z);
    // Update zoom towards target, dt in seconds
    void UpdateZoom(double dt);

    // Build orthonormal basis vectors representing the camera rig in world space.
    // When includePitchInForward is false, the forward basis ignores pitch for horizontal moves.
    Basis BuildBasis(bool includePitchInForward = true) const noexcept;

private:
    static double ClampFov(double fov) noexcept;

    double x_;
    double y_;
    double z_;
    double pitch_;
    double yaw_;
    double zoom_;
    double targetZoom_;
};
