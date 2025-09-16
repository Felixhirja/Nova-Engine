#include "Viewport3D.h"
#include <iostream>

Viewport3D::Viewport3D() : width(800), height(600) {}

Viewport3D::~Viewport3D() {}

void Viewport3D::Init() {
    std::cout << "Viewport3D Initialized with size " << width << "x" << height << std::endl;
}

void Viewport3D::Render() {
    // Rendering happens here (silenced in console to reduce spam).
}

void Viewport3D::DrawPlayer(double x) {
    // Simple ASCII drawing: map x in [-5,5] to 40-character line
    const int widthChars = 40;
    int center = widthChars / 2;
    double clamped = x;
    if (clamped < -5.0) clamped = -5.0;
    if (clamped > 5.0) clamped = 5.0;
    int pos = static_cast<int>((clamped + 5.0) / 10.0 * (widthChars - 1));
    std::string line(widthChars, '-');
    line[pos] = 'P';
    std::cout << line << std::endl;
}

void Viewport3D::Resize(int w, int h) {
    width = w;
    height = h;
    std::cout << "Viewport3D Resized to " << width << "x" << height << std::endl;
}
