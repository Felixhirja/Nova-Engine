#pragma once

#include <glad/glad.h>

#include <cstddef>
#include <unordered_map>
#include <vector>

// SpriteMetadataBuffer manages a GPU buffer containing sprite-sheet metadata.
// Metadata is uploaded only when sprites are registered/updated and reused every frame.
class SpriteMetadataBuffer {
public:
    SpriteMetadataBuffer();
    ~SpriteMetadataBuffer();

    // Explicit initialization. Safe to call multiple times.
    bool Init();
    void Shutdown();

    // Register or update metadata for the given sprite handle.
    void UpdateSprite(int handle,
                      int frameWidth,
                      int frameHeight,
                      int frameCount,
                      int fps,
                      int textureWidth,
                      int textureHeight);

    // Upload data to GPU if there were changes since the last upload.
    void UploadPending();

    // Bind the buffer to the supplied uniform block binding point.
    void Bind(GLuint bindingPoint) const;

    GLuint GetBufferID() const { return bufferId_; }
    size_t GetMetadataCount() const { return data_.size(); }
    int GetIndexForHandle(int handle) const;
    bool IsInitialized() const { return initialized_; }

private:
    struct alignas(16) SpriteMetadataGPU {
        // frameInfo: [frameWidth, frameHeight, uvScaleX, uvScaleY]
        float frameInfo[4];
        // textureInfo: [textureWidth, textureHeight, frameCount, fps]
        float textureInfo[4];
    };

    bool EnsureInitialized();

    GLuint bufferId_;
    bool initialized_;
    bool initAttempted_;
    bool initFailed_;
    bool dirty_;

    std::vector<SpriteMetadataGPU> data_;
    std::unordered_map<int, size_t> handleToIndex_;
};

