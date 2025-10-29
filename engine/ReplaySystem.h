#pragma once

#include "DeterministicRandom.h"
#include "ecs/EntityManager.h"
#include "ecs/Components.h"

#include <string>
#include <vector>

struct PlayerInputSnapshot {
    bool forward = false;
    bool backward = false;
    bool up = false;
    bool down = false;
    bool strafeLeft = false;
    bool strafeRight = false;
    bool sprint = false;
    bool crouch = false;
    bool slide = false;
    bool boost = false;
    bool left = false;
    bool right = false;
    double cameraYaw = camera_defaults::kDefaultYawRadians;
};

struct EntityPhysicsSnapshot {
    unsigned int entity = 0u;
    Position position;
    Velocity velocity;
};

struct ReplayFrame {
    double timestamp = 0.0;
    PlayerInputSnapshot input;
    DeterministicRandom::StreamState randomState;
    std::vector<EntityPhysicsSnapshot> entities;
};

class DeterministicReplayRecorder {
public:
    void StartRecording(uint64_t seed);
    void StopRecording();
    bool IsRecording() const { return recording_; }

    void RecordFrame(double timestamp,
                     const PlayerInputSnapshot& input,
                     const DeterministicRandom::StreamState& randomState,
                     EntityManager& entityManager);

    bool SaveToFile(const std::string& path) const;
    const std::vector<ReplayFrame>& GetFrames() const { return frames_; }

private:
    bool recording_ = false;
    uint64_t baseSeed_ = 0u;
    std::vector<ReplayFrame> frames_;
};

class DeterministicReplayPlayer {
public:
    bool LoadFromFile(const std::string& path);
    void SetFrames(std::vector<ReplayFrame> frames);

    void BeginPlayback();
    void StopPlayback();
    bool IsPlaying() const { return playing_; }

    const ReplayFrame* ConsumeNextFrame();
    void ApplyFrameToEntities(const ReplayFrame& frame, EntityManager& entityManager) const;

private:
    std::vector<ReplayFrame> frames_;
    size_t nextFrameIndex_ = 0u;
    bool playing_ = false;
};

