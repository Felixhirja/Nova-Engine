#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct SDL_Surface;

struct SvgRasterizationOptions {
    // Explicit output width/height in pixels. A value of 0 leaves the dimension
    // unconstrained and derives it from the SVG document size. When both values
    // are zero, the SVG's native size is used (optionally scaled by `scale`).
    int targetWidth = 0;
    int targetHeight = 0;

    // Uniform scaling factor applied when neither target dimension is
    // specified. Values <= 0 fallback to 1.0f.
    float scale = 1.0f;

    // When true and at least one target dimension is provided, the aspect
    // ratio defined by the SVG document is preserved. If both dimensions are
    // provided the smaller scale that fits inside the requested rectangle is
    // used. If false, each dimension is scaled independently.
    bool preserveAspectRatio = true;
};

// Loads an SVG file and rasterizes it into a 32-bit RGBA SDL surface.
// Returns nullptr on failure. The caller owns the returned surface and must
// destroy it via compat_DestroySurface / SDL_FreeSurface depending on SDL
// version.
SDL_Surface* LoadSVGSurface(const std::string& path,
                            SvgRasterizationOptions options = {});

// Rasterizes an SVG file into a tightly packed RGBA buffer.
// Returns true on success and fills outPixels with width*height*4 bytes.
bool LoadSvgToRgba(const std::string& path,
                   std::vector<std::uint8_t>& outPixels,
                   int& outWidth,
                   int& outHeight,
                   SvgRasterizationOptions options = {});
