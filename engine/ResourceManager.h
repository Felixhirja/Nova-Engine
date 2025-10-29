#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#ifdef USE_GLFW
class SpriteMetadataBuffer;
#endif

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

    // Simple stub: load returns an integer handle (incremental)
    int Load(const std::string &path);
    bool Exists(int handle) const;

    void Shutdown();

    // Sprite-sheet metadata: frame size and count
    struct SpriteInfo { int frameW; int frameH; int frames; int fps; };
    void RegisterSprite(int handle, const SpriteInfo& info);
    bool GetSpriteInfo(int handle, SpriteInfo &out) const;

#ifdef USE_GLFW
    SpriteMetadataBuffer* GetSpriteMetadataBuffer();
    const SpriteMetadataBuffer* GetSpriteMetadataBuffer() const;
    void SyncSpriteMetadataGPU();
#endif

#ifdef USE_SDL
    // Return cached SDL_Surface* if available (ResourceManager owns the surface)
    void* GetSurface(int handle) const;
    // Return (and cache) an SDL_Texture* created for the given renderer and handle
    void* GetTexture(void* renderer, int handle);
#endif

private:
    int nextHandle_ = 1;
    std::unordered_map<int, std::string> map_;
#ifdef USE_SDL
    // cache loaded surfaces
    std::unordered_map<int, void*> surfaces_;
    // cache textures per renderer pointer
    std::unordered_map<void*, std::unordered_map<int, void*>> textures_;
    // sprite metadata
    std::unordered_map<int, SpriteInfo> spriteInfo_;
#endif
#ifdef USE_GLFW
    std::unique_ptr<SpriteMetadataBuffer> spriteMetadataBuffer_;
#endif
    bool shutdown_ = false;

    void EnsureActive();
};
