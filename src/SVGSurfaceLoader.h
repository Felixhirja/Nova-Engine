#pragma once

#include <string>

struct SDL_Surface;

// Loads an SVG file and rasterizes it into a 32-bit RGBA SDL surface.
// Returns nullptr on failure. The caller owns the returned surface and must
// destroy it via compat_DestroySurface / SDL_FreeSurface depending on SDL
// version.
SDL_Surface* LoadSVGSurface(const std::string& path);
