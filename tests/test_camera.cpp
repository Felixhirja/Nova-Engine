#include "../src/Camera.h"
#include <cassert>
#include <iostream>

int main() {
    Camera c(0.0, 0.0, 1.0, 0.0, 0.0, 1.0);
    int sx, sy;
    // center should map to center
    c.WorldToScreen(0.0, 0.0, 0.0, 800, 600, sx, sy);
    assert(sx == 400 && sy == 300);

    // one unit to the right should move by zoom pixels
    c.SetZoom(10.0);
    c.WorldToScreen(1.0, 0.0, 0.0, 800, 600, sx, sy);
    assert(sx == 400 + 10);

    // move camera center, world point at camera center should map to center
    c.MoveTo(5.0, -3.0, 1.0);
    c.SetZoom(2.0);
    c.WorldToScreen(5.0, -3.0, 1.0, 800, 600, sx, sy);
    assert(sx == 400 && sy == 300);

    std::cout << "Camera tests passed" << std::endl;
    return 0;
}
