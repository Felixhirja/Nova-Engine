#include "SpriteMetadataBuffer.h"

#include <algorithm>
#include <iostream>

SpriteMetadataBuffer::SpriteMetadataBuffer()
    : bufferId_(0)
    , initialized_(false)
    , initAttempted_(false)
    , initFailed_(false)
    , dirty_(false) {}

SpriteMetadataBuffer::~SpriteMetadataBuffer() {
    Shutdown();
}

bool SpriteMetadataBuffer::Init() {
    return EnsureInitialized();
}

void SpriteMetadataBuffer::Shutdown() {
    if (bufferId_ != 0) {
        glDeleteBuffers(1, &bufferId_);
        bufferId_ = 0;
    }
    initialized_ = false;
    initAttempted_ = false;
    initFailed_ = false;
    dirty_ = false;
    data_.clear();
    handleToIndex_.clear();
}

bool SpriteMetadataBuffer::EnsureInitialized() {
    if (initialized_) {
        return true;
    }

    if (initAttempted_) {
        return !initFailed_;
    }

    initAttempted_ = true;

    if (glGenBuffers == nullptr || glBindBuffer == nullptr || glBufferData == nullptr || glBindBufferBase == nullptr) {
        std::cerr << "SpriteMetadataBuffer: required OpenGL functions not available" << std::endl;
        initFailed_ = true;
        return false;
    }

    glGenBuffers(1, &bufferId_);
    if (bufferId_ == 0) {
        std::cerr << "SpriteMetadataBuffer: glGenBuffers returned 0" << std::endl;
        initFailed_ = true;
        return false;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, bufferId_);
    glBufferData(GL_UNIFORM_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    initialized_ = true;
    initFailed_ = false;
    dirty_ = true; // ensure first upload initializes buffer contents
    return true;
}

void SpriteMetadataBuffer::UpdateSprite(int handle,
                                        int frameWidth,
                                        int frameHeight,
                                        int frameCount,
                                        int fps,
                                        int textureWidth,
                                        int textureHeight) {
    if (!EnsureInitialized()) {
        return;
    }

    SpriteMetadataGPU metadata{};

    const int safeFrameCount = std::max(frameCount, 1);
    const int safeTextureWidth = textureWidth > 0 ? textureWidth : frameWidth * safeFrameCount;
    const int safeTextureHeight = textureHeight > 0 ? textureHeight : frameHeight;

    const float texWidthF = static_cast<float>(safeTextureWidth);
    const float texHeightF = static_cast<float>(safeTextureHeight);
    const float frameWidthF = static_cast<float>(frameWidth);
    const float frameHeightF = static_cast<float>(frameHeight);

    metadata.frameInfo[0] = frameWidthF;
    metadata.frameInfo[1] = frameHeightF;
    metadata.frameInfo[2] = texWidthF > 0.0f ? frameWidthF / texWidthF : 0.0f;
    metadata.frameInfo[3] = texHeightF > 0.0f ? frameHeightF / texHeightF : 0.0f;

    metadata.textureInfo[0] = texWidthF;
    metadata.textureInfo[1] = texHeightF;
    metadata.textureInfo[2] = static_cast<float>(safeFrameCount);
    metadata.textureInfo[3] = static_cast<float>(std::max(fps, 0));

    auto it = handleToIndex_.find(handle);
    if (it == handleToIndex_.end()) {
        size_t newIndex = data_.size();
        handleToIndex_[handle] = newIndex;
        data_.push_back(metadata);
    } else {
        data_[it->second] = metadata;
    }

    dirty_ = true;
}

void SpriteMetadataBuffer::UploadPending() {
    if (!EnsureInitialized() || !dirty_) {
        return;
    }

    glBindBuffer(GL_UNIFORM_BUFFER, bufferId_);
    glBufferData(GL_UNIFORM_BUFFER,
                 static_cast<GLsizeiptr>(data_.size() * sizeof(SpriteMetadataGPU)),
                 data_.empty() ? nullptr : data_.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    dirty_ = false;
}

void SpriteMetadataBuffer::Bind(GLuint bindingPoint) const {
    if (!initialized_ || bufferId_ == 0) {
        return;
    }

    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, bufferId_);
}

int SpriteMetadataBuffer::GetIndexForHandle(int handle) const {
    auto it = handleToIndex_.find(handle);
    if (it == handleToIndex_.end()) {
        return -1;
    }
    return static_cast<int>(it->second);
}

