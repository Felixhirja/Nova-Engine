#include "ReplaySystem.h"

#include <fstream>

void DeterministicReplayRecorder::StartRecording(uint64_t seed) {
    baseSeed_ = seed;
    frames_.clear();
    recording_ = true;
}

void DeterministicReplayRecorder::StopRecording() {
    recording_ = false;
}

void DeterministicReplayRecorder::RecordFrame(double timestamp,
                                              const PlayerInputSnapshot& input,
                                              const DeterministicRandom::StreamState& randomState,
                                              EntityManager& entityManager) {
    if (!recording_) {
        return;
    }

    ReplayFrame frame;
    frame.timestamp = timestamp;
    frame.input = input;
    frame.randomState = randomState;

    auto positions = entityManager.GetAllWith<Position>();
    for (auto& [entity, position] : positions) {
        EntityPhysicsSnapshot snapshot;
        snapshot.entity = entity;
        snapshot.position = *position;
        if (auto* velocity = entityManager.GetComponent<Velocity>(entity)) {
            snapshot.velocity = *velocity;
        }
        frame.entities.push_back(snapshot);
    }

    frames_.push_back(std::move(frame));
}

bool DeterministicReplayRecorder::SaveToFile(const std::string& path) const {
    if (frames_.empty()) {
        return false;
    }

    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }

    file << "#nova_replay\n";
    file << "seed " << baseSeed_ << "\n";
    for (const auto& frame : frames_) {
        file << "frame " << frame.timestamp << ' ' << frame.randomState.seed << ' ' << frame.randomState.draws << '\n';
        file << "input "
             << frame.input.forward << ' '
             << frame.input.backward << ' '
             << frame.input.up << ' '
             << frame.input.down << ' '
             << frame.input.strafeLeft << ' '
             << frame.input.strafeRight << ' '
             << frame.input.sprint << ' '
             << frame.input.crouch << ' '
             << frame.input.slide << ' '
             << frame.input.boost << ' '
             << frame.input.left << ' '
             << frame.input.right << ' '
             << frame.input.cameraYaw << '\n';
        for (const auto& entity : frame.entities) {
            file << "entity " << entity.entity << ' '
                 << entity.position.x << ' ' << entity.position.y << ' ' << entity.position.z << ' '
                 << entity.velocity.vx << ' ' << entity.velocity.vy << ' ' << entity.velocity.vz << '\n';
        }
        file << "endframe\n";
    }

    return true;
}

bool DeterministicReplayPlayer::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    std::vector<ReplayFrame> frames;
    std::string token;
    uint64_t seed = 0u;
    while (file >> token) {
        if (token == "#nova_replay") {
            continue;
        } else if (token == "seed") {
            file >> seed;
        } else if (token == "frame") {
            ReplayFrame frame;
            file >> frame.timestamp;
            file >> frame.randomState.seed;
            file >> frame.randomState.draws;
            std::string inner;
            while (file >> inner) {
                if (inner == "input") {
                    file >> frame.input.forward
                         >> frame.input.backward
                         >> frame.input.up
                         >> frame.input.down
                         >> frame.input.strafeLeft
                         >> frame.input.strafeRight
                         >> frame.input.sprint
                         >> frame.input.crouch
                         >> frame.input.slide
                         >> frame.input.boost
                         >> frame.input.left
                         >> frame.input.right
                         >> frame.input.cameraYaw;
                } else if (inner == "entity") {
                    EntityPhysicsSnapshot snapshot;
                    file >> snapshot.entity
                         >> snapshot.position.x >> snapshot.position.y >> snapshot.position.z
                         >> snapshot.velocity.vx >> snapshot.velocity.vy >> snapshot.velocity.vz;
                    frame.entities.push_back(snapshot);
                } else if (inner == "endframe") {
                    break;
                } else {
                    // Unknown token; stop parsing this frame
                    break;
                }
            }
            frames.push_back(std::move(frame));
        }
    }

    if (frames.empty()) {
        return false;
    }

    SetFrames(std::move(frames));
    return true;
}

void DeterministicReplayPlayer::SetFrames(std::vector<ReplayFrame> frames) {
    frames_ = std::move(frames);
    nextFrameIndex_ = 0u;
}

void DeterministicReplayPlayer::BeginPlayback() {
    playing_ = !frames_.empty();
    nextFrameIndex_ = 0u;
}

void DeterministicReplayPlayer::StopPlayback() {
    playing_ = false;
    nextFrameIndex_ = 0u;
}

const ReplayFrame* DeterministicReplayPlayer::ConsumeNextFrame() {
    if (!playing_ || nextFrameIndex_ >= frames_.size()) {
        playing_ = false;
        return nullptr;
    }
    const ReplayFrame* frame = &frames_[nextFrameIndex_];
    ++nextFrameIndex_;
    if (nextFrameIndex_ >= frames_.size()) {
        playing_ = false;
    }
    return frame;
}

void DeterministicReplayPlayer::ApplyFrameToEntities(const ReplayFrame& frame, EntityManager& entityManager) const {
    for (const auto& entity : frame.entities) {
        if (!entityManager.IsAlive(entity.entity)) {
            continue;
        }
        if (auto* pos = entityManager.GetComponent<Position>(entity.entity)) {
            *pos = entity.position;
        }
        if (auto* vel = entityManager.GetComponent<Velocity>(entity.entity)) {
            *vel = entity.velocity;
        }
    }
}

