#include "../src/FramePacingController.h"

#include <cassert>
#include <cmath>
#include <limits>

namespace {
constexpr double kEpsilon = 1e-9;

bool ApproximatelyEqual(double a, double b) {
    return std::abs(a - b) < kEpsilon;
}
} // namespace

int main() {
    FramePacingController controller;

    // VSync toggling and explicit enable/disable.
    assert(!controller.IsVSyncEnabled());
    controller.SetVSyncEnabled(true);
    assert(controller.IsVSyncEnabled());
    controller.ToggleVSync();
    assert(!controller.IsVSyncEnabled());

    // Target FPS clamping behavior.
    controller.SetTargetFPS(72.0);
    assert(ApproximatelyEqual(controller.TargetFPS(), 72.0));

    controller.SetTargetFPS(-5.0);
    assert(ApproximatelyEqual(controller.TargetFPS(), 0.0));

    controller.SetTargetFPS(1000.0);
    assert(ApproximatelyEqual(controller.TargetFPS(), 360.0));

    // AdjustTargetFPS delegates to SetTargetFPS and clamps the result.
    controller.SetTargetFPS(144.0);
    controller.AdjustTargetFPS(-44.0);
    assert(ApproximatelyEqual(controller.TargetFPS(), 100.0));

    controller.AdjustTargetFPS(-250.0);
    assert(ApproximatelyEqual(controller.TargetFPS(), 0.0));

    controller.AdjustTargetFPS(400.0);
    assert(ApproximatelyEqual(controller.TargetFPS(), 360.0));

    // Desired frame duration should be the inverse of the target FPS when positive.
    controller.SetTargetFPS(120.0);
    auto duration = controller.DesiredFrameDuration();
    assert(ApproximatelyEqual(duration.count(), 1.0 / 120.0));

    controller.SetTargetFPS(0.0);
    assert(ApproximatelyEqual(controller.DesiredFrameDuration().count(), 0.0));

    // NaN and infinity inputs should be ignored.
    controller.SetTargetFPS(120.0);
    controller.SetTargetFPS(std::numeric_limits<double>::quiet_NaN());
    assert(ApproximatelyEqual(controller.TargetFPS(), 120.0));

    controller.SetTargetFPS(std::numeric_limits<double>::infinity());
    assert(ApproximatelyEqual(controller.TargetFPS(), 120.0));

    controller.AdjustTargetFPS(std::numeric_limits<double>::infinity());
    assert(ApproximatelyEqual(controller.TargetFPS(), 120.0));

    return 0;
}
