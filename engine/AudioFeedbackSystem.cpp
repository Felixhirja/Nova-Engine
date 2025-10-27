#include "AudioFeedbackSystem.h"
#include "AudioSystem.h"
#include <iostream>
#include <cmath>

AudioFeedbackSystem::AudioFeedbackSystem() {}

AudioFeedbackSystem::~AudioFeedbackSystem() {
    Shutdown();
}

bool AudioFeedbackSystem::Initialize() {
    if (initialized_) {
        return true;
    }
    
    // Initialize AudioSystem
    if (!AudioSystem::Initialize()) {
        std::cerr << "AudioFeedbackSystem: Failed to initialize AudioSystem" << std::endl;
        return false;
    }
    
    // Register default audio clips
    RegisterDefaultClips();
    
    initialized_ = true;
    // std::cout << "Audio feedback system initialized" << std::endl;
    return true;
}

void AudioFeedbackSystem::Shutdown() {
    if (!initialized_) {
        return;
    }
    
    StopAllSounds();
    clips_.clear();
    
    // AudioSystem will be shut down by MainLoop or manually
    // We don't shut it down here in case other systems need it
    
    initialized_ = false;
    // std::cout << "Audio feedback system shut down" << std::endl;
}

void AudioFeedbackSystem::RegisterDefaultClips() {
    // Shield sounds
    RegisterClip("shield_hit", "assets/audio/shield_hit.wav", 0.7f);
    RegisterClip("shield_depleted", "assets/audio/shield_depleted.wav", 0.9f);
    RegisterClip("shield_recharge", "assets/audio/shield_recharge.wav", 0.5f);
    
    // Hull/damage sounds
    RegisterClip("hull_impact", "assets/audio/hull_impact.wav", 0.8f);
    RegisterClip("sparks", "assets/audio/sparks.wav", 0.6f);
    RegisterClip("explosion", "assets/audio/explosion.wav", 1.0f);
    RegisterClip("subsystem_failure", "assets/audio/subsystem_failure.wav", 0.8f);
    
    // Weapon sounds
    RegisterClip("weapon_fire", "assets/audio/weapon_fire.wav", 0.7f);
    RegisterClip("weapon_overheat", "assets/audio/weapon_overheat.wav", 0.6f);
    RegisterClip("ammo_empty", "assets/audio/ammo_empty.wav", 0.5f);
    
    // Alarm sounds (looping)
    RegisterClip("alarm_warning", "assets/audio/alarm_warning.wav", 0.6f);
    RegisterClip("alarm_critical", "assets/audio/alarm_critical.wav", 0.8f);
    RegisterClip("alarm_evacuate", "assets/audio/alarm_evacuate.wav", 1.0f);
    
    // UI sounds
    RegisterClip("power_diverted", "assets/audio/power_diverted.wav", 0.5f);
    RegisterClip("beep_low", "assets/audio/beep_low.wav", 0.4f);
    RegisterClip("beep_high", "assets/audio/beep_high.wav", 0.4f);
}

void AudioFeedbackSystem::SetListenerPosition(double x, double y, double z) {
    listenerX_ = x;
    listenerY_ = y;
    listenerZ_ = z;
    
    // Update AudioSystem listener position
    AudioSystem::SetListenerPosition(x, y, z);
}

void AudioFeedbackSystem::SetListenerOrientation(double forwardX, double forwardY, double forwardZ,
                                                double upX, double upY, double upZ) {
    // Update AudioSystem listener orientation
    AudioSystem::SetListenerOrientation(forwardX, forwardY, forwardZ, upX, upY, upZ);
}

void AudioFeedbackSystem::RegisterClip(const std::string& name, const std::string& filePath, float volume) {
    AudioClip clip;
    clip.filePath = filePath;
    clip.volume = volume;
    clips_[name] = clip;
}

int AudioFeedbackSystem::PlaySound(const std::string& clipName, bool loop) {
    return PlaySound3D(clipName, 0.0, 0.0, 0.0, loop);
}

int AudioFeedbackSystem::PlaySound3D(const std::string& clipName, double x, double y, double z, bool loop) {
    auto it = clips_.find(clipName);
    if (it == clips_.end()) {
        std::cerr << "Audio clip not found: " << clipName << std::endl;
        return -1;
    }
    
    // Load clip if not already cached
    if (it->second.clipId < 0) {
        it->second.clipId = AudioSystem::LoadSound(it->second.filePath);
        if (it->second.clipId < 0) {
            return -1; // Failed to load
        }
    }
    
    return PlayClip(it->second, true, x, y, z);
}

int AudioFeedbackSystem::PlayClip(const AudioClip& clip, bool is3D, double x, double y, double z) {
    if (!initialized_) {
        return -1;
    }
    
    // Play sound using AudioSystem
    AudioSystem::SoundHandle handle;
    if (is3D) {
        handle = AudioSystem::PlaySound3D(clip.clipId, x, y, z, clip.loop, clip.volume);
    } else {
        handle = AudioSystem::PlaySound(clip.clipId, clip.loop, clip.volume);
    }
    
    if (!handle.IsValid()) {
        return -1;
    }
    
    // Track the playback
    int soundId = nextSoundId_++;
    AudioPlayback playback;
    playback.soundId = soundId;
    playback.channelId = handle.channelId;
    playback.clipId = handle.clipId;
    playback.x = x;
    playback.y = y;
    playback.z = z;
    playback.volume = clip.volume * sfxVolume_ * masterVolume_;
    playback.isPlaying = true;
    playback.is3D = is3D;
    
    activePlaybacks_[soundId] = playback;
    
    return soundId;
}

void AudioFeedbackSystem::StopSound(int soundId) {
    auto it = activePlaybacks_.find(soundId);
    if (it != activePlaybacks_.end()) {
        // Stop via AudioSystem
        AudioSystem::SoundHandle handle;
        handle.channelId = it->second.channelId;
        handle.clipId = it->second.clipId;
        AudioSystem::StopSound(handle);
        
        it->second.isPlaying = false;
        activePlaybacks_.erase(it);
    }
}

void AudioFeedbackSystem::StopAllSounds() {
    activePlaybacks_.clear();
    criticalAlarmSoundId_ = -1;
    warningAlarmSoundId_ = -1;
    
    // Stop all sounds via AudioSystem
    AudioSystem::StopAllSounds();
}

void AudioFeedbackSystem::SetMasterVolume(float volume) {
    masterVolume_ = std::max(0.0f, std::min(1.0f, volume));
    AudioSystem::SetMasterVolume(masterVolume_);
}

void AudioFeedbackSystem::SetSFXVolume(float volume) {
    sfxVolume_ = std::max(0.0f, std::min(1.0f, volume));
    AudioSystem::SetSFXVolume(sfxVolume_);
}

void AudioFeedbackSystem::SetAlarmVolume(float volume) {
    alarmVolume_ = std::max(0.0f, std::min(1.0f, volume));
    // Alarm volume could be treated as separate category, or as part of SFX
    // For now, we'll just store it but use SFX volume for actual playback
}

void AudioFeedbackSystem::OnFeedbackEvent(const FeedbackEvent& event) {
    if (!initialized_) {
        return;
    }
    
    switch (event.type) {
        case FeedbackEventType::ShieldHit:
            PlaySound3D("shield_hit", event.x, event.y, event.z);
            break;
            
        case FeedbackEventType::ShieldDepleted:
            PlaySound3D("shield_depleted", event.x, event.y, event.z);
            break;
            
        case FeedbackEventType::ShieldRecharging:
            PlaySound("shield_recharge");
            break;
            
        case FeedbackEventType::HullDamage:
            PlaySound3D("hull_impact", event.x, event.y, event.z);
            if (event.magnitude > 20.0) {
                PlaySound3D("sparks", event.x, event.y, event.z);
            }
            break;
            
        case FeedbackEventType::CriticalDamage:
            PlaySound3D("explosion", event.x, event.y, event.z);
            break;
            
        case FeedbackEventType::SubsystemFailure:
            PlaySound3D("subsystem_failure", event.x, event.y, event.z);
            break;
            
        case FeedbackEventType::WeaponFired:
            PlaySound3D("weapon_fire", event.x, event.y, event.z);
            break;
            
        case FeedbackEventType::WeaponOverheat:
            PlaySound("weapon_overheat");
            break;
            
        case FeedbackEventType::AmmoEmpty:
            PlaySound("ammo_empty");
            break;
            
        case FeedbackEventType::EnergyDiverted:
            PlaySound("power_diverted");
            break;
            
        case FeedbackEventType::WarningLowShields:
            if (warningAlarmSoundId_ == -1) {
                warningAlarmSoundId_ = PlaySound("alarm_warning", true);
            }
            break;
            
        case FeedbackEventType::AlarmCritical:
            if (criticalAlarmSoundId_ == -1) {
                criticalAlarmSoundId_ = PlaySound("alarm_critical", true);
            }
            break;
            
        case FeedbackEventType::AlarmEvacuate:
            StopAllSounds();
            PlaySound("alarm_evacuate", true);
            break;
            
        default:
            break;
    }
}
