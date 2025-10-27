#include "AnimationSystem.h"
#include "EntityManager.h"
#include "Components.h"

#include <algorithm>

namespace {
constexpr double kMinimumFrameDuration = 1e-6;
}

void AnimationSystem::Update(EntityManager& entityManager, double dt) {
    if (dt <= 0.0) {
        return;
    }

    entityManager.ForEach<AnimationState, Sprite>(
        [&](Entity /*entity*/, AnimationState& animation, Sprite& sprite) {
            if (!animation.playing) {
                // Ensure sprite stays synchronized even when paused
                sprite.frame = animation.currentFrame;
                return;
            }

            const int rangeStart = animation.startFrame;
            const int rangeEnd = std::max(animation.endFrame, rangeStart);
            const int frameCount = rangeEnd - rangeStart + 1;
            const double frameDuration = std::max(animation.frameDuration, kMinimumFrameDuration);
            if (animation.playbackDirection == 0) {
                animation.playbackDirection = 1;
            }

            // Keep the current frame within the configured range
            if (animation.currentFrame < rangeStart || animation.currentFrame > rangeEnd) {
                animation.currentFrame = rangeStart;
                animation.frameTimer = 0.0;
            }

            animation.frameTimer += dt;

            while (animation.frameTimer >= frameDuration) {
                animation.frameTimer -= frameDuration;
                animation.currentFrame += animation.playbackDirection;

                if (animation.currentFrame > rangeEnd) {
                    if (animation.pingPong && frameCount > 1) {
                        animation.currentFrame = rangeEnd - 1;
                        animation.playbackDirection = -1;
                    } else if (animation.looping) {
                        animation.currentFrame = rangeStart;
                    } else {
                        animation.currentFrame = rangeEnd;
                        animation.playing = false;
                        animation.frameTimer = 0.0;
                        break;
                    }
                } else if (animation.currentFrame < rangeStart) {
                    if (animation.pingPong && frameCount > 1) {
                        animation.currentFrame = rangeStart + 1;
                        animation.playbackDirection = 1;
                    } else if (animation.looping) {
                        animation.currentFrame = rangeEnd;
                    } else {
                        animation.currentFrame = rangeStart;
                        animation.playing = false;
                        animation.frameTimer = 0.0;
                        break;
                    }
                }
            }

            if (!animation.looping && !animation.pingPong) {
                if (animation.playbackDirection >= 0 && animation.currentFrame >= rangeEnd) {
                    animation.currentFrame = rangeEnd;
                    animation.playing = false;
                    animation.frameTimer = 0.0;
                } else if (animation.playbackDirection < 0 && animation.currentFrame <= rangeStart) {
                    animation.currentFrame = rangeStart;
                    animation.playing = false;
                    animation.frameTimer = 0.0;
                }
            }

            sprite.frame = animation.currentFrame;
        });
}
