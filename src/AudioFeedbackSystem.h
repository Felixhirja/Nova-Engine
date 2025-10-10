#pragma once

#include "FeedbackEvent.h"
#include <string>
#include <unordered_map>
#include <memory>

// Audio clip representation
struct AudioClip {
    std::string filePath;
    float volume = 1.0f;
    bool loop = false;
    float pitch = 1.0f;
    int clipId = -1; // AudioSystem clip ID (cached)
};

// Audio playback state
struct AudioPlayback {
    int soundId; // Our internal ID
    int channelId; // AudioSystem channel ID
    int clipId; // AudioSystem clip ID
    double x, y, z;  // 3D position
    float volume;
    bool isPlaying;
    bool is3D;
};

// Audio feedback system with spatial audio support
class AudioFeedbackSystem : public IFeedbackListener {
public:
    AudioFeedbackSystem();
    ~AudioFeedbackSystem();
    
    // Initialize audio system
    bool Initialize();
    void Shutdown();
    
    // Update listener position for 3D audio
    void SetListenerPosition(double x, double y, double z);
    void SetListenerOrientation(double forwardX, double forwardY, double forwardZ,
                               double upX, double upY, double upZ);
    
    // Audio playback
    int PlaySound(const std::string& clipName, bool loop = false);
    int PlaySound3D(const std::string& clipName, double x, double y, double z, bool loop = false);
    void StopSound(int soundId);
    void StopAllSounds();
    
    // Master volume control
    void SetMasterVolume(float volume);
    void SetSFXVolume(float volume);
    void SetAlarmVolume(float volume);
    
    // IFeedbackListener implementation
    void OnFeedbackEvent(const FeedbackEvent& event) override;
    
    // Register audio clips
    void RegisterClip(const std::string& name, const std::string& filePath, float volume = 1.0f);
    
private:
    bool initialized_ = false;
    int nextSoundId_ = 1;
    
    // Audio clips registry
    std::unordered_map<std::string, AudioClip> clips_;
    
    // Active playbacks
    std::unordered_map<int, AudioPlayback> activePlaybacks_;
    
    // Volume settings
    float masterVolume_ = 1.0f;
    float sfxVolume_ = 1.0f;
    float alarmVolume_ = 1.0f;
    
    // Listener position
    double listenerX_ = 0.0, listenerY_ = 0.0, listenerZ_ = 0.0;
    
    // Looping alarm states
    int criticalAlarmSoundId_ = -1;
    int warningAlarmSoundId_ = -1;
    
    // Helper methods
    int PlayClip(const AudioClip& clip, bool is3D, double x, double y, double z);
    void RegisterDefaultClips();
};
