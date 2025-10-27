#include "ResourceManager.h"
#ifdef USE_SDL
#if defined(USE_SDL3)
#include <SDL3/SDL.h>
#elif defined(USE_SDL2)
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif
#include "sdl_compat.h"
#include "SVGSurfaceLoader.h"
#endif
#ifdef USE_GLFW
#include "graphics/SpriteMetadataBuffer.h"
#endif
#include <algorithm>
#include <cctype>
#include <mutex>

ResourceManager::ResourceManager() {
#ifdef USE_GLFW
    spriteMetadataBuffer_ = std::make_unique<SpriteMetadataBuffer>();
#endif
}

ResourceManager::~ResourceManager() {
#ifdef USE_SDL
    for (auto &p : surfaces_) {
#if SDL_MAJOR_VERSION >= 3
        if (p.second) compat_DestroySurface(static_cast<SDL_Surface*>(p.second));
#else
        if (p.second) SDL_FreeSurface(static_cast<SDL_Surface*>(p.second));
#endif
    }
    // free textures
    for (auto &r : textures_) {
        for (auto &p : r.second) {
            if (p.second) SDL_DestroyTexture(static_cast<SDL_Texture*>(p.second));
        }
    }
#endif
}

int ResourceManager::Load(const std::string &path) {
    int h = nextHandle_++;
    map_[h] = path;
#ifdef USE_SDL
    // Do not load now; lazy load on request
#endif
    return h;
}

bool ResourceManager::Exists(int handle) const {
    return map_.find(handle) != map_.end();
}

#ifdef USE_SDL
void* ResourceManager::GetSurface(int handle) const {
    auto it = map_.find(handle);
    if (it == map_.end()) return nullptr;
    auto sit = surfaces_.find(handle);
    if (sit != surfaces_.end()) return sit->second;
    // not found: load now (const_cast for simplicity since surfaces_ is mutable cache)
    const std::string &path = it->second;
    auto hasExtension = [](const std::string& str, const std::string& ext) {
        if (str.size() < ext.size()) return false;
        auto it1 = str.end();
        auto it2 = ext.end();
        while (it2 != ext.begin()) {
            --it1; --it2;
            if (std::tolower(static_cast<unsigned char>(*it1)) != std::tolower(static_cast<unsigned char>(*it2))) {
                return false;
            }
        }
        return true;
    };

    SDL_Surface* s = nullptr;
    if (hasExtension(path, ".svg")) {
        s = LoadSVGSurface(path);
    }
    if (!s) {
        s = compat_LoadBMP(path.c_str());
    }
    if (!s) return nullptr;
    // Store in non-const map via const_cast (ok for this simple demo)
    const_cast<std::unordered_map<int, void*>&>(surfaces_)[handle] = static_cast<void*>(s);
    return static_cast<void*>(s);
}

void* ResourceManager::GetTexture(void* rendererPtr, int handle) {
    if (!rendererPtr) return nullptr;
    auto it = map_.find(handle);
    if (it == map_.end()) return nullptr;
    // check texture cache
    auto &rendererMap = textures_[rendererPtr];
    auto tit = rendererMap.find(handle);
    if (tit != rendererMap.end()) return tit->second;
    // create texture: ensure surface exists
    SDL_Surface* surf = static_cast<SDL_Surface*>(GetSurface(handle));
    if (!surf) return nullptr;
    SDL_Renderer* ren = static_cast<SDL_Renderer*>(rendererPtr);
    SDL_Texture* tex = compat_CreateTextureFromSurface(ren, surf);
    if (!tex) return nullptr;
    rendererMap[handle] = static_cast<void*>(tex);
    return static_cast<void*>(tex);
}
#endif

void ResourceManager::RegisterSprite(int handle, const SpriteInfo& info) {
#ifdef USE_SDL
    spriteInfo_[handle] = info;
#else
    (void)handle; (void)info;
#endif
#ifdef USE_GLFW
    if (spriteMetadataBuffer_) {
        int textureWidth = info.frameW * std::max(1, info.frames);
        int textureHeight = info.frameH;
#ifdef USE_SDL
        if (void* surfRaw = GetSurface(handle)) {
            SDL_Surface* surf = static_cast<SDL_Surface*>(surfRaw);
            if (surf) {
                textureWidth = surf->w;
                textureHeight = surf->h;
            }
        }
#endif
        spriteMetadataBuffer_->UpdateSprite(handle,
                                            info.frameW,
                                            info.frameH,
                                            info.frames,
                                            info.fps,
                                            textureWidth,
                                            textureHeight);
    }
#endif
}

bool ResourceManager::GetSpriteInfo(int handle, SpriteInfo &out) const {
#ifdef USE_SDL
    auto it = spriteInfo_.find(handle);
    if (it == spriteInfo_.end()) return false;
    out = it->second;
    return true;
#else
    (void)handle; (void)out; return false;
#endif
}

#ifdef USE_GLFW
SpriteMetadataBuffer* ResourceManager::GetSpriteMetadataBuffer() {
    return spriteMetadataBuffer_.get();
}

const SpriteMetadataBuffer* ResourceManager::GetSpriteMetadataBuffer() const {
    return spriteMetadataBuffer_.get();
}

void ResourceManager::SyncSpriteMetadataGPU() {
    if (spriteMetadataBuffer_) {
        spriteMetadataBuffer_->UploadPending();
    }
}
#endif
