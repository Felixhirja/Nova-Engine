#include "ecs/AnimationSystem.h"
#include "ecs/Components.h"
#include "ecs/EntityManager.h"

#include <cassert>
#include <memory>

int main() {
    EntityManager entityManager;
    AnimationSystem animationSystem;

    // Looping animation advances frames and wraps
    Entity loopingEntity = entityManager.CreateEntity();
    auto loopingSprite = std::make_shared<Sprite>();
    entityManager.AddComponent<Sprite>(loopingEntity, loopingSprite);
    auto loopingAnimation = std::make_shared<AnimationState>();
    loopingAnimation->startFrame = 0;
    loopingAnimation->endFrame = 3;
    loopingAnimation->currentFrame = 0;
    loopingAnimation->frameDuration = 0.1;
    loopingAnimation->looping = true;
    loopingAnimation->playing = true;
    entityManager.AddComponent<AnimationState>(loopingEntity, loopingAnimation);

    animationSystem.Update(entityManager, 0.05);
    assert(loopingSprite->frame == 0);
    animationSystem.Update(entityManager, 0.05);
    assert(loopingSprite->frame == 1);
    animationSystem.Update(entityManager, 0.4);
    assert(loopingSprite->frame == 1);
    assert(loopingAnimation->currentFrame == 1);

    // Non-looping animation stops on the last frame
    Entity nonLoopEntity = entityManager.CreateEntity();
    auto nonLoopSprite = std::make_shared<Sprite>();
    entityManager.AddComponent<Sprite>(nonLoopEntity, nonLoopSprite);
    auto nonLoopAnimation = std::make_shared<AnimationState>();
    nonLoopAnimation->startFrame = 5;
    nonLoopAnimation->endFrame = 6;
    nonLoopAnimation->currentFrame = 5;
    nonLoopAnimation->frameDuration = 0.1;
    nonLoopAnimation->looping = false;
    nonLoopAnimation->playing = true;
    entityManager.AddComponent<AnimationState>(nonLoopEntity, nonLoopAnimation);

    animationSystem.Update(entityManager, 0.1);
    assert(nonLoopSprite->frame == 6);
    assert(!nonLoopAnimation->playing);
    animationSystem.Update(entityManager, 1.0);
    assert(nonLoopSprite->frame == 6);

    // Ping-pong animation reverses direction
    Entity pingPongEntity = entityManager.CreateEntity();
    auto pingPongSprite = std::make_shared<Sprite>();
    entityManager.AddComponent<Sprite>(pingPongEntity, pingPongSprite);
    auto pingPongAnimation = std::make_shared<AnimationState>();
    pingPongAnimation->startFrame = 0;
    pingPongAnimation->endFrame = 2;
    pingPongAnimation->currentFrame = 0;
    pingPongAnimation->frameDuration = 0.1;
    pingPongAnimation->looping = true;
    pingPongAnimation->pingPong = true;
    pingPongAnimation->playing = true;
    entityManager.AddComponent<AnimationState>(pingPongEntity, pingPongAnimation);

    animationSystem.Update(entityManager, 0.1);
    assert(pingPongSprite->frame == 1);
    animationSystem.Update(entityManager, 0.1);
    assert(pingPongSprite->frame == 2);
    animationSystem.Update(entityManager, 0.1);
    assert(pingPongSprite->frame == 1);

    return 0;
}
