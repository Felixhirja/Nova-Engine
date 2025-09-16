#pragma once

class Camera {
public:
    Camera();
    Camera(double x, double y, double zoom);

    void SetPosition(double x, double y);
    void SetZoom(double z);

    // Smoothly move camera towards target over time (lerp)
    void LerpTo(double targetX, double targetY, double alpha);
    // Instant move
    void MoveTo(double x, double y);

    // Convert world coords to screen coords (centered camera)
    void WorldToScreen(double wx, double wy, int screenW, int screenH, int &outX, int &outY) const;

    double x() const { return x_; }
    double y() const { return y_; }
    double zoom() const { return zoom_; }
    double targetZoom() const { return targetZoom_; }
    void SetTargetZoom(double z);
    // Update zoom towards target, dt in seconds
    void UpdateZoom(double dt);

private:
    double x_;
    double y_;
    double zoom_;
    double targetZoom_ = 1.0;
};
