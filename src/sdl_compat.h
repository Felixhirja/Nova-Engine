// Minimal SDL2->SDL3 compatibility helpers used by Nova-Engine
#pragma once
#ifdef USE_SDL
#define SDL_WINDOW_SHOWN ((SDL_WindowFlags)0x00000004)
#define SDL_WINDOW_OPENGL ((SDL_WindowFlags)0x00000002)
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#include <algorithm>
#include <cstring>
#include <vector>
// Convert SDL_Rect (int) to SDL_FRect (float)
static inline SDL_FRect sdl_rect_to_frect(const SDL_Rect* r) {
    SDL_FRect fr;
    if (r) {
        fr.x = (float)r->x;
        fr.y = (float)r->y;
        fr.w = (float)r->w;
        fr.h = (float)r->h;
    } else {
        fr.x = fr.y = fr.w = fr.h = 0.0f;
    }
    return fr;
}

// Wrapper for SDL_RenderFillRect (SDL2) -> SDL_RenderFillRect(SDL_FRect*) in SDL3
static inline bool compat_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect) {
    if (!rect) return SDL_RenderFillRect(renderer, nullptr);
    SDL_FRect fr = sdl_rect_to_frect(rect);
    return SDL_RenderFillRect(renderer, &fr) == 0 ? true : true;
}

static inline bool compat_RenderFillRects(SDL_Renderer* renderer, const SDL_Rect* rects, int count) {
    if (!rects || count <= 0) return false;
    std::vector<SDL_FRect> fr(count);
    for (int i = 0; i < count; ++i) fr[i] = sdl_rect_to_frect(&rects[i]);
    return SDL_RenderFillRects(renderer, fr.data(), count);
}

// Wrapper for SDL_RenderDrawRect
static inline bool compat_RenderDrawRect(SDL_Renderer* renderer, const SDL_Rect* rect) {
    if (!rect) return SDL_RenderRect(renderer, nullptr);
    SDL_FRect fr = sdl_rect_to_frect(rect);
    return SDL_RenderRect(renderer, &fr) == 0 ? true : true;
}

// Wrapper for draw line (SDL2 uses int coords)
static inline bool compat_RenderDrawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2) {
    return SDL_RenderLine(renderer, (float)x1, (float)y1, (float)x2, (float)y2);
}

// Wrapper for SDL_RenderCopy -> SDL_RenderTexture
static inline bool compat_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect) {
    SDL_FRect s_buf, d_buf;
    const SDL_FRect* s = nullptr; const SDL_FRect* d = nullptr;
    if (srcrect) { s_buf = sdl_rect_to_frect(srcrect); s = &s_buf; }
    if (dstrect) { d_buf = sdl_rect_to_frect(dstrect); d = &d_buf; }
    // SDL_RenderTexture returns bool in SDL3
    return SDL_RenderTexture(renderer, texture, s, d);
}

// SDL_RenderReadPixels changed in SDL3: it returns an SDL_Surface*
// We implement compat_RenderReadPixels to fill the provided buffer in RGB24 format
static inline int compat_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect, Uint32 format, void* pixels, int pitch) {
    // SDL3: SDL_RenderReadPixels returns an SDL_Surface* owned by the caller
    SDL_Surface* surf = SDL_RenderReadPixels(renderer, rect ? rect : nullptr);
    if (!surf) return -1;
    // surf->format is an SDL_PixelFormat enum (value), compare against requested
    if (format != surf->format) {
        // Convert surface to the requested pixel format
    SDL_Surface* conv = SDL_ConvertSurface(surf, (SDL_PixelFormat)format);
        SDL_DestroySurface(surf);
        if (!conv) return -1;
        surf = conv;
    }
    // Copy rows respecting pitch and source pitch
    int h = surf->h;
    int srcPitch = surf->pitch;
    unsigned char* src = (unsigned char*)surf->pixels;
    unsigned char* dst = (unsigned char*)pixels;
    for (int y = 0; y < h; ++y) {
        std::memcpy(dst + (size_t)y * (size_t)pitch, src + (size_t)y * (size_t)srcPitch, (size_t)std::min(pitch, srcPitch));
    }
    SDL_DestroySurface(surf);
    return 0;
}

static inline void compat_DestroySurface(SDL_Surface* surface) { SDL_DestroySurface(surface); }
static inline SDL_Surface* compat_LoadBMP(const char* file) { return SDL_LoadBMP(file); }
static inline SDL_Texture* compat_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) { return SDL_CreateTextureFromSurface(renderer, surface); }
static inline SDL_Window* compat_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags) {
#if defined(USE_SDL3)
    return SDL_CreateWindow(title, w, h, flags);
#else
    return SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, (Uint32)flags);
#endif
}
static inline SDL_Renderer* compat_CreateRenderer(SDL_Window* window, const char* name) {
#if defined(USE_SDL3)
    return SDL_CreateRenderer(window, name);
#else
    SDL_Renderer* r = SDL_CreateRenderer(window, 0, SDL_RENDERER_ACCELERATED);
    if (!r) {
        r = SDL_CreateRenderer(window, 0, SDL_RENDERER_SOFTWARE);
    }
    return r;
#endif
}
static inline void compat_GL_DeleteContext(SDL_GLContext context) {
#if defined(USE_SDL3)
    SDL_GL_DestroyContext(context);
#else
    SDL_GL_DeleteContext(context);
#endif
}

static inline void* compat_GetWindowNativeHandle(SDL_Window* window) {
#if defined(_WIN32)
    if (!window) {
        return nullptr;
    }
    SDL_PropertiesID props = SDL_GetWindowProperties(window);
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#else
    (void)window;
    return nullptr;
#endif
}
#else // USE_SDL2
// On SDL2 we can map compat_* to the SDL2 originals
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
static inline bool compat_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect) { return SDL_RenderFillRect(renderer, rect) == 0 ? true : true; }
static inline bool compat_RenderFillRects(SDL_Renderer* renderer, const SDL_Rect* rects, int count) { return SDL_RenderFillRects(renderer, rects, count) == 0 ? true : true; }
static inline bool compat_RenderDrawRect(SDL_Renderer* renderer, const SDL_Rect* rect) { return SDL_RenderDrawRect(renderer, rect) == 0 ? true : true; }
static inline bool compat_RenderDrawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2) { SDL_RenderDrawLine(renderer, x1,y1,x2,y2); return true; }
static inline bool compat_RenderCopy(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Rect* srcrect, const SDL_Rect* dstrect) { SDL_RenderCopy(renderer, texture, srcrect, dstrect); return true; }
static inline int compat_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect, Uint32 format, void* pixels, int pitch) { return SDL_RenderReadPixels(renderer, rect, format, pixels, pitch); }
static inline void compat_DestroySurface(SDL_Surface* surface) { SDL_FreeSurface(surface); }
static inline SDL_Surface* compat_LoadBMP(const char* file) { return SDL_LoadBMP(file); }
static inline SDL_Texture* compat_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface) { return SDL_CreateTextureFromSurface(renderer, surface); }
static inline SDL_Window* compat_CreateWindow(const char* title, int w, int h, SDL_WindowFlags flags) {
    return SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w, h, (Uint32)flags);
}
static inline SDL_Renderer* compat_CreateRenderer(SDL_Window* window, const char* name) {
    return SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
}
static inline void compat_GL_DeleteContext(SDL_GLContext context) {
    SDL_GL_DeleteContext(context);
}

static inline void* compat_GetWindowNativeHandle(SDL_Window* window) {
#if defined(_WIN32)
    if (!window) {
        return nullptr;
    }
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    if (SDL_GetWindowWMInfo(window, &wmInfo)) {
        return wmInfo.info.win.window;
    }
    return nullptr;
#else
    (void)window;
    return nullptr;
#endif
}
#endif
#endif
