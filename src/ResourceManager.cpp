#include "ResourceManager.h"
#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif
#include <mutex>

ResourceManager::ResourceManager() {}

ResourceManager::~ResourceManager() {
#ifdef USE_SDL
    for (auto &p : surfaces_) {
        if (p.second) SDL_FreeSurface(static_cast<SDL_Surface*>(p.second));
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
    SDL_Surface* s = SDL_LoadBMP(path.c_str());
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
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
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
