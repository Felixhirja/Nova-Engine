#ifndef VIEWPORT3D_H
#define VIEWPORT3D_H

class Viewport3D {
public:
    Viewport3D();
    ~Viewport3D();

    void Init();
    void Render();
    void Resize(int width, int height);
    // Draw a player at world x coordinate (for simple ASCII demo)
    void DrawPlayer(double x);

private:
    int width;
    int height;
};

#endif // VIEWPORT3D_H
