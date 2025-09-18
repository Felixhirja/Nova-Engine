#include "Viewport3D.h"
#include <iostream>
#include <vector>
#ifdef USE_SDL
#include <SDL2/SDL.h>
#include <cstdint>
#include "ResourceManager.h"
#include "Camera.h"
#endif

Viewport3D::Viewport3D() : width(800), height(600), usingSDL(false)
#ifdef USE_SDL
, sdlWindow(nullptr), sdlRenderer(nullptr)
#endif
{}

Viewport3D::~Viewport3D() {}

void Viewport3D::Init() {
    usingSDL = false;
#ifdef USE_SDL
        if (SDL_Init(SDL_INIT_VIDEO) == 0) {
            sdlWindow = SDL_CreateWindow("Star Engine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 0);
            if (sdlWindow) {
                sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
                if (sdlRenderer) {
                    usingSDL = true;
                    std::cout << "Viewport3D: Using SDL2 for rendering." << std::endl;
                    return;
                } else {
                    std::cerr << "Viewport3D: SDL_CreateRenderer failed: " << SDL_GetError() << std::endl;
                }
            } else {
                std::cerr << "Viewport3D: SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
            }
            if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
            if (sdlWindow) SDL_DestroyWindow(sdlWindow);
            SDL_Quit();
        } else {
            std::cerr << "Viewport3D: SDL_Init failed: " << SDL_GetError() << std::endl;
    }
#endif
    std::cout << "Viewport3D Initialized with size " << width << "x" << height << " (ASCII fallback)" << std::endl;
}

void Viewport3D::Render() {
    if (usingSDL) {
#ifdef USE_SDL
        Clear();
        Present();
#endif
    } else {
        // ASCII fallback: nothing to do here
    }
}

void Viewport3D::Clear() {
    if (!usingSDL) return;
#ifdef USE_SDL
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);
#endif
}

void Viewport3D::Present() {
    if (!usingSDL) return;
#ifdef USE_SDL
    SDL_RenderPresent(sdlRenderer);
#endif
}

void Viewport3D::DrawPlayer(double x) {
    if (usingSDL) {
#ifdef USE_SDL
        int px = static_cast<int>(((x + 5.0) / 10.0) * width);
        int py = height / 2;
        SDL_Rect rect{px - 5, py - 5, 10, 10};
        SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 0, 255);
        SDL_RenderFillRect(sdlRenderer, &rect);
#endif
    } else {
        const int widthChars = 40;
        double clamped = x;
        if (clamped < -5.0) clamped = -5.0;
        if (clamped > 5.0) clamped = 5.0;
        int pos = static_cast<int>((clamped + 5.0) / 10.0 * (widthChars - 1));
        std::string line(widthChars, '-');
        line[pos] = 'P';
        std::cout << line << std::endl;
    }
}

void Viewport3D::DrawEntity(const Transform &t) {
    // Very simple: use the entity's x coordinate and draw like DrawPlayer
    DrawPlayer(t.x);
}

void Viewport3D::DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, int currentFrame) {
    // Forward to camera-aware overload (camera=nullptr)
    DrawEntity(t, textureHandle, resourceManager, nullptr, currentFrame);
}

void Viewport3D::DrawEntity(const Transform &t, int textureHandle, class ResourceManager* resourceManager, const class Camera* camera, int currentFrame) {
    if (usingSDL) {
#ifdef USE_SDL
        int px, py;
        if (camera) {
            camera->WorldToScreen(t.x, t.y, width, height, px, py);
        } else {
            px = static_cast<int>(((t.x + 5.0) / 10.0) * width);
            py = height / 2;
        }
        int w = 16, h = 16;
        SDL_Rect dst{px - w/2, py - h/2, w, h};

        // Try texture path first
        if (textureHandle != 0 && resourceManager) {
            void* texRaw = resourceManager->GetTexture(static_cast<void*>(sdlRenderer), textureHandle);
            if (texRaw) {
                SDL_Texture* tex = static_cast<SDL_Texture*>(texRaw);
                // If sprite info available, compute source rect
                ResourceManager::SpriteInfo info;
                SDL_Rect srcRect;
                bool haveSrc = false;
                if (resourceManager->GetSpriteInfo(textureHandle, info)) {
                    int frame = currentFrame;
                    srcRect.x = frame * info.frameW;
                    srcRect.y = 0;
                    srcRect.w = info.frameW;
                    srcRect.h = info.frameH;
                    haveSrc = true;
                }
                SDL_RenderCopy(sdlRenderer, tex, haveSrc ? &srcRect : nullptr, &dst);
                return;
            }
        }

        // Fallback: draw an orange rectangle
        SDL_SetRenderDrawColor(sdlRenderer, 255, 128, 0, 255);
        SDL_RenderFillRect(sdlRenderer, &dst);
#endif
    } else {
        DrawPlayer(t.x);
    }
}

void Viewport3D::Resize(int w, int h) {
    width = w; height = h;
    std::cout << "Viewport3D Resized to " << width << "x" << height << std::endl;
}

void Viewport3D::Shutdown() {
    if (usingSDL) {
#ifdef USE_SDL
        if (sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
        if (sdlWindow) SDL_DestroyWindow(sdlWindow);
        SDL_Quit();
#endif
        usingSDL = false;
    }
}

void Viewport3D::DrawCameraMarker(const class Camera* camera) {
    if (!usingSDL) return;
#ifdef USE_SDL
    if (!sdlRenderer || !camera) return;
    // Draw a small cross at the center of the screen
    int cx = width / 2;
    int cy = height / 2;
    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 255, 255);
    SDL_RenderDrawLine(sdlRenderer, cx - 8, cy, cx + 8, cy);
    SDL_RenderDrawLine(sdlRenderer, cx, cy - 8, cx, cy + 8);
#endif
}

#ifdef USE_SDL
// Simple 3x5 pixel font for ASCII 0-9, '-', '.' (monochrome) drawn as small rectangles
static const uint8_t tinyFont[][5] = {
    {0x1F,0x11,0x11,0x11,0x1F}, // 0
    {0x04,0x06,0x04,0x04,0x07}, // 1
    {0x1F,0x01,0x1F,0x10,0x1F}, // 2
    {0x1F,0x01,0x1F,0x01,0x1F}, // 3
    {0x11,0x11,0x1F,0x01,0x01}, // 4
    {0x1F,0x10,0x1F,0x01,0x1F}, // 5
    {0x1F,0x10,0x1F,0x11,0x1F}, // 6
    {0x1F,0x01,0x02,0x04,0x04}, // 7
    {0x1F,0x11,0x1F,0x11,0x1F}, // 8
    {0x1F,0x11,0x1F,0x01,0x1F}, // 9
};

// Larger tiny font renderer with a few extra glyphs (Z, F, X, =)
static void drawTinyCharSDL(SDL_Renderer* r, int x, int y, char c) {
    if (!r) return;
    const int scale = 8; // increase for much better visibility
    // helper to draw a 5x5 glyph stored as 5 columns
    auto drawGlyph = [&](const uint8_t* glyph){
        for (int col = 0; col < 5; ++col) {
            uint8_t colBits = glyph[col];
            for (int row = 0; row < 5; ++row) {
                if (colBits & (1 << (4 - row))) {
                    SDL_Rect px{ x + col*(scale+1), y + row*(scale+1), scale, scale };
                    SDL_RenderFillRect(r, &px);
                }
            }
        }
    };

    if (c >= '0' && c <= '9') {
        const uint8_t* glyph = tinyFont[c - '0'];
        drawGlyph(glyph);
    } else if (c == '-') {
        SDL_Rect px{ x, y + 2*(scale+1), 5*(scale+1), scale };
        SDL_RenderFillRect(r, &px);
    } else if (c == '.') {
        SDL_Rect px{ x + 4*(scale+1), y + 4*(scale+1), scale, scale };
        SDL_RenderFillRect(r, &px);
    } else if (c == '=') {
        SDL_Rect top{ x, y + 1*(scale+1), 5*(scale+1), scale };
        SDL_Rect bot{ x, y + 3*(scale+1), 5*(scale+1), scale };
        SDL_RenderFillRect(r, &top);
        SDL_RenderFillRect(r, &bot);
    } else if (c == 'Z') {
        const uint8_t glyphZ[5] = {0x1F, 0x02, 0x04, 0x08, 0x1F};
        drawGlyph(glyphZ);
    } else if (c == 'F') {
        const uint8_t glyphF[5] = {0x1F, 0x10, 0x1E, 0x10, 0x10};
        drawGlyph(glyphF);
    } else if (c == 'P') {
        const uint8_t glyphP[5] = {0x1F, 0x11, 0x1F, 0x10, 0x10};
        drawGlyph(glyphP);
    } else if (c == 'S') {
        const uint8_t glyphS[5] = {0x1F, 0x10, 0x1F, 0x01, 0x1F};
        drawGlyph(glyphS);
    } else if (c == 'X') {
        const uint8_t glyphX[5] = {0x11, 0x0A, 0x04, 0x0A, 0x11};
        drawGlyph(glyphX);
    } else if (c == ':') {
        // draw two dots for colon
        SDL_Rect d1{ x + 2*(scale+1), y + 1*(scale+1), scale/2, scale/2 };
        SDL_Rect d2{ x + 2*(scale+1), y + 3*(scale+1), scale/2, scale/2 };
        SDL_RenderFillRect(r, &d1);
        SDL_RenderFillRect(r, &d2);
    } else if (c == ' ' || c == ':') {
        // leave blank for space/colon (handled below if needed)
    } else {
        // unknown: leave blank
    }
}

// Draw a single seven-segment style digit at (x,y).
static void drawSevenSegDigit(SDL_Renderer* r, int x, int y, int segLen, int segThick, char c) {
    if (!r) return;
    // segment bitmask: bit0=a(top), bit1=b(upper-right), bit2=c(lower-right), bit3=d(bottom), bit4=e(lower-left), bit5=f(upper-left), bit6=g(middle)
    static const uint8_t segMap[10] = {
        // a b c d e f g
        0b0111111, // 0: a b c d e f
        0b0000110, // 1: b c
        0b1011011, // 2: a b g e d
        0b1001111, // 3: a b g c d
        0b1100110, // 4: f g b c
        0b1101101, // 5: a f g c d
        0b1111101, // 6: a f g e c d
        0b0000111, // 7: a b c
        0b1111111, // 8: all
        0b1101111  // 9: a b c d f g
    };

    auto drawSeg = [&](int sx, int sy, int w, int h){ SDL_Rect rct{sx, sy, w, h}; SDL_RenderFillRect(r, &rct); };

    int a = x + segThick, ay = y, aw = segLen, ah = segThick;
    int f_x = x, f_y = y + segThick, f_w = segThick, f_h = segLen;
    int b_x = x + segThick + segLen, b_y = y + segThick, b_w = segThick, b_h = segLen;
    int g_x = x + segThick, g_y = y + segThick + segLen, g_w = segLen, g_h = segThick;
    int e_x = x, e_y = y + 2*segThick + segLen, e_w = segThick, e_h = segLen;
    int c_x = x + segThick + segLen, c_y = y + 2*segThick + segLen, c_w = segThick, c_h = segLen;
    int d_x = x + segThick, d_y = y + 2*(segThick + segLen), d_w = segLen, d_h = segThick;

    if (c == '-') {
        drawSeg(g_x, g_y, g_w, g_h);
        return;
    }
    if (c == '.') {
        // small dot at bottom-right
        SDL_Rect dot{ x + segThick + segLen + segThick/2, y + 2*(segThick+segLen) + segThick, segThick, segThick };
        SDL_RenderFillRect(r, &dot);
        return;
    }

    if (c < '0' || c > '9') return;
    uint8_t bits = segMap[c - '0'];
    if (bits & 0x01) drawSeg(a, ay, aw, ah); // a
    if (bits & 0x02) drawSeg(b_x, b_y, b_w, b_h); // b
    if (bits & 0x04) drawSeg(c_x, c_y, c_w, c_h); // c
    if (bits & 0x08) drawSeg(d_x, d_y, d_w, d_h); // d
    if (bits & 0x10) drawSeg(e_x, e_y, e_w, e_h); // e
    if (bits & 0x20) drawSeg(f_x, f_y, f_w, f_h); // f
    if (bits & 0x40) drawSeg(g_x, g_y, g_w, g_h); // g
}
#endif // USE_SDL

void Viewport3D::DrawHUD(const class Camera* camera, double fps, double playerX) {
    if (!usingSDL) return;
#ifdef USE_SDL
    if (!sdlRenderer) return;
    // Draw semi-transparent background box with border
    SDL_SetRenderDrawBlendMode(sdlRenderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 180);
    SDL_Rect bg{8, 8, 380, 120};
    SDL_RenderFillRect(sdlRenderer, &bg);
    // white border
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 180);
    SDL_RenderDrawRect(sdlRenderer, &bg);

    // HUD text color: bright white for digits
    SDL_SetRenderDrawColor(sdlRenderer, 245, 245, 245, 255);

    // Layout for seven-seg digits (larger for clarity)
    int segLen = 16;
    int segThick = 6;
    int spacing = segLen + segThick + 8;
    int x = 18, y = 18;

    // Draw label "Z:" using small rectangles (simple readable glyph)
    SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
    // draw 'Z' as a simple diagonal-ish using rects
    SDL_Rect rz1{ x, y, 4, segThick }; SDL_RenderFillRect(sdlRenderer, &rz1);
    SDL_Rect rz2{ x + segLen - 2, y + segThick, 4, segThick }; SDL_RenderFillRect(sdlRenderer, &rz2);
    SDL_Rect rz3{ x, y + 2*(segThick + segLen), 4, segThick }; SDL_RenderFillRect(sdlRenderer, &rz3);
    x += 24;

    // Draw Zoom numeric value (one digit before decimal, one after) as "xx.x"
    char zbuf[32];
    if (camera) snprintf(zbuf, sizeof(zbuf), "%.1f", camera->zoom()); else snprintf(zbuf, sizeof(zbuf), "0.0");
    SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
    for (char* p = zbuf; *p; ++p) {
        if (*p >= '0' && *p <= '9') {
            drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
            x += spacing;
        } else if (*p == '.') {
            drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '.');
            x += spacing/2;
        }
    }

    x += 18; // gap before FPS
    // FPS label and digits
    SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
    // draw "FPS:" label as three small rects
    SDL_Rect rf1{ x, y, 6, segThick }; SDL_RenderFillRect(sdlRenderer, &rf1);
    SDL_Rect rf2{ x, y + segThick + 2, 6, segThick }; SDL_RenderFillRect(sdlRenderer, &rf2);
    SDL_Rect rf3{ x, y + 2*(segThick+segLen), 6, segThick }; SDL_RenderFillRect(sdlRenderer, &rf3);
    x += 18;
    char fbuf[16]; snprintf(fbuf, sizeof(fbuf), "%d", (int)std::floor(fps + 0.5));
    SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
    for (char* p = fbuf; *p; ++p) {
        drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
        x += spacing;
    }

    x += 18;
    // Player X label
    SDL_SetRenderDrawColor(sdlRenderer, 200, 200, 200, 255);
    SDL_Rect rx1{ x, y, 6, segThick }; SDL_RenderFillRect(sdlRenderer, &rx1);
    SDL_Rect rx2{ x + 8, y + segThick + 2, 6, segThick }; SDL_RenderFillRect(sdlRenderer, &rx2);
    x += 18;
    // Player X numeric with 2 decimals (we draw only digits and decimal point)
    char xbuf[32]; snprintf(xbuf, sizeof(xbuf), "%.2f", playerX);
    SDL_SetRenderDrawColor(sdlRenderer, 255, 230, 120, 255);
    for (char* p = xbuf; *p; ++p) {
        if (*p >= '0' && *p <= '9') {
            drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, *p);
            x += spacing;
        } else if (*p == '.') {
            drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '.');
            x += spacing/2;
        } else if (*p == '-') {
            // draw minus sign as middle seg
            drawSevenSegDigit(sdlRenderer, x, y, segLen, segThick, '-');
            x += spacing;
        }
    }
#endif
}

bool Viewport3D::CaptureToBMP(const char* path) {
#ifdef USE_SDL
    if (!usingSDL || !sdlRenderer) return false;
    // Read pixels from current render target
    int w = width, h = height;
    int pitch = w * 3;
    std::vector<unsigned char> pixels(pitch * h);
    // Ensure render target is the default
    if (SDL_RenderReadPixels(sdlRenderer, nullptr, SDL_PIXELFORMAT_RGB24, pixels.data(), pitch) != 0) {
        std::cerr << "Viewport3D::CaptureToBMP: SDL_RenderReadPixels failed: " << SDL_GetError() << std::endl;
        return false;
    }

    // BMP 24-bit header
    int rowBytes = ((w * 3 + 3) / 4) * 4;
    int imgSize = rowBytes * h;
    unsigned char header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    int fileSize = 54 + imgSize;
    header[2] = (unsigned char)(fileSize & 0xFF); header[3] = (unsigned char)((fileSize>>8) & 0xFF);
    header[4] = (unsigned char)((fileSize>>16) & 0xFF); header[5] = (unsigned char)((fileSize>>24) & 0xFF);
    header[10] = 54; header[14] = 40;
    header[18] = (unsigned char)(w & 0xFF); header[19] = (unsigned char)((w>>8) & 0xFF);
    header[22] = (unsigned char)(h & 0xFF); header[23] = (unsigned char)((h>>8) & 0xFF);
    header[26] = 1; header[28] = 24;

    FILE* f = fopen(path, "wb");
    if (!f) return false;
    fwrite(header, 1, 54, f);
    // BMP stores rows bottom-up
    std::vector<unsigned char> row(rowBytes);
    for (int y = h - 1; y >= 0; --y) {
        unsigned char* src = pixels.data() + y * pitch;
        int idx = 0;
        for (int x = 0; x < w; ++x) {
            // source is RGB; BMP needs BGR
            row[idx++] = src[x*3 + 2];
            row[idx++] = src[x*3 + 1];
            row[idx++] = src[x*3 + 0];
        }
        while (idx < rowBytes) row[idx++] = 0;
        fwrite(row.data(), 1, rowBytes, f);
    }
    fclose(f);
    return true;
#else
    (void)path;
    return false;
#endif
}
