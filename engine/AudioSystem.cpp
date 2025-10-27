#include "AudioSystem.h"
#include <iostream>
#include <cmath>
#include <algorithm>

// Define M_PI if not available (Windows)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef USE_SDL_MIXER
#if defined(USE_SDL3)
#include <SDL3_mixer/SDL_mixer.h>
#elif defined(USE_SDL2)
#include <SDL2/SDL_mixer.h>
#else
#include <SDL_mixer/SDL_mixer.h>
#endif
#endif

// ========== Static Member Initialization ==========

bool AudioSystem::initialized_ = false;
int AudioSystem::nextClipId_ = 1;

#ifdef USE_SDL_MIXER
std::unordered_map<int, Mix_Chunk*> AudioSystem::soundClips_;
void* AudioSystem::currentMusic_ = nullptr; // Actually Mix_Music*
#endif

std::unordered_map<std::string, int> AudioSystem::soundPaths_;
std::string AudioSystem::currentMusicPath_;

float AudioSystem::masterVolume_ = 1.0f;
float AudioSystem::sfxVolume_ = 1.0f;
float AudioSystem::musicVolume_ = 1.0f;

double AudioSystem::listenerX_ = 0.0;
double AudioSystem::listenerY_ = 0.0;
double AudioSystem::listenerZ_ = 0.0;
double AudioSystem::listenerForwardX_ = 0.0;
double AudioSystem::listenerForwardY_ = 0.0;
double AudioSystem::listenerForwardZ_ = 1.0;
double AudioSystem::listenerUpX_ = 0.0;
double AudioSystem::listenerUpY_ = 1.0;
double AudioSystem::listenerUpZ_ = 0.0;

double AudioSystem::maxAudioDistance_ = 100.0;
double AudioSystem::referenceDistance_ = 1.0;
double AudioSystem::rolloffFactor_ = 1.0;

// ========== Initialization ==========

bool AudioSystem::Initialize() {
    if (initialized_) {
        // std::cout << "AudioSystem: Already initialized" << std::endl;
        return true;
    }
    
#ifdef USE_SDL_MIXER
    // Initialize SDL_mixer
    // Frequency: 44100 Hz, Format: 16-bit signed, Channels: Stereo, Chunk size: 2048 bytes
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "AudioSystem: Failed to initialize SDL_mixer - " << Mix_GetError() << std::endl;
        return false;
    }
    
    // Allocate 32 mixing channels for sound effects (default is 8)
    Mix_AllocateChannels(32);
    
    // Set initial volumes (0-128 range for SDL_mixer)
    Mix_Volume(-1, MIX_MAX_VOLUME);  // All channels
    Mix_VolumeMusic(MIX_MAX_VOLUME);
    
    // std::cout << "AudioSystem: Initialized successfully" << std::endl;
    // std::cout << "  - Frequency: 44100 Hz" << std::endl;
    // std::cout << "  - Channels: 32 (stereo output)" << std::endl;
    // std::cout << "  - Chunk size: 2048 bytes" << std::endl;
    
    initialized_ = true;
    return true;
#else
    std::cerr << "AudioSystem: SDL_mixer not available (USE_SDL_MIXER not defined)" << std::endl;
    return false;
#endif
}

void AudioSystem::Shutdown() {
    if (!initialized_) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    // Stop all sounds and music
    StopAllSounds();
    StopMusic(0.0f);
    
    // Free all loaded sound clips
    for (auto& pair : soundClips_) {
        if (pair.second) {
            Mix_FreeChunk(pair.second);
        }
    }
    soundClips_.clear();
    soundPaths_.clear();
    
    // Free music
    if (currentMusic_) {
        Mix_FreeMusic(static_cast<Mix_Music*>(currentMusic_));
        currentMusic_ = nullptr;
    }
    currentMusicPath_.clear();
    
    // Close SDL_mixer
    Mix_CloseAudio();
    
    // std::cout << "AudioSystem: Shut down successfully" << std::endl;
#endif
    
    initialized_ = false;
}

bool AudioSystem::IsInitialized() {
    return initialized_;
}

// ========== Sound Effects ==========

int AudioSystem::LoadSound(const std::string& filePath) {
    if (!initialized_) {
        std::cerr << "AudioSystem: Cannot load sound, system not initialized" << std::endl;
        return -1;
    }
    
    // Check if already loaded
    auto it = soundPaths_.find(filePath);
    if (it != soundPaths_.end()) {
        return it->second; // Return existing clip ID
    }
    
#ifdef USE_SDL_MIXER
    // Load the sound file
    Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
    if (!chunk) {
        std::cerr << "AudioSystem: Failed to load sound '" << filePath << "' - " << Mix_GetError() << std::endl;
        return -1;
    }
    
    // Assign a new clip ID
    int clipId = nextClipId_++;
    soundClips_[clipId] = chunk;
    soundPaths_[filePath] = clipId;
    
    // std::cout << "AudioSystem: Loaded sound '" << filePath << "' (clipId=" << clipId << ")" << std::endl;
    return clipId;
#else
    std::cerr << "AudioSystem: SDL_mixer not available" << std::endl;
    return -1;
#endif
}

AudioSystem::SoundHandle AudioSystem::PlaySound(int clipId, bool loop, float volume) {
    return PlaySound3D(clipId, listenerX_, listenerY_, listenerZ_, loop, volume);
}

AudioSystem::SoundHandle AudioSystem::PlaySound3D(int clipId, double x, double y, double z, 
                                                   bool loop, float volume) {
    if (!initialized_) {
        std::cerr << "AudioSystem: Cannot play sound, system not initialized" << std::endl;
        return {-1, clipId};
    }
    
#ifdef USE_SDL_MIXER
    // Find the clip
    auto it = soundClips_.find(clipId);
    if (it == soundClips_.end() || !it->second) {
        std::cerr << "AudioSystem: Invalid clip ID " << clipId << std::endl;
        return {-1, clipId};
    }
    
    Mix_Chunk* chunk = it->second;
    
    // Calculate 3D audio parameters
    float attenuation = CalculateDistanceAttenuation(x, y, z);
    int pan = CalculateStereoPan(x, y, z);
    
    // Play sound (-1 = first available channel, loops: -1 = infinite, 0 = once)
    int loops = loop ? -1 : 0;
    int channel = Mix_PlayChannel(-1, chunk, loops);
    
    if (channel < 0) {
        std::cerr << "AudioSystem: Failed to play sound - " << Mix_GetError() << std::endl;
        return {-1, clipId};
    }
    
    // Apply volume (base * attenuation * SFX * master)
    float finalVolume = volume * attenuation * sfxVolume_ * masterVolume_;
    int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
    sdlVolume = std::max(0, std::min(MIX_MAX_VOLUME, sdlVolume));
    Mix_Volume(channel, sdlVolume);
    
    // Apply stereo panning
    Mix_SetPanning(channel, 255 - pan, pan); // Left, Right (inverted because SDL_mixer is weird)
    
    return {channel, clipId};
#else
    return {-1, clipId};
#endif
}

void AudioSystem::StopSound(SoundHandle handle) {
    if (!initialized_ || handle.channelId < 0) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    Mix_HaltChannel(handle.channelId);
#endif
}

void AudioSystem::StopAllSounds() {
    if (!initialized_) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    Mix_HaltChannel(-1); // -1 = all channels
#endif
}

bool AudioSystem::IsSoundPlaying(SoundHandle handle) {
    if (!initialized_ || handle.channelId < 0) {
        return false;
    }
    
#ifdef USE_SDL_MIXER
    return Mix_Playing(handle.channelId) != 0;
#else
    return false;
#endif
}

void AudioSystem::SetSoundVolume(SoundHandle handle, float volume) {
    if (!initialized_ || handle.channelId < 0) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    float finalVolume = volume * sfxVolume_ * masterVolume_;
    int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
    sdlVolume = std::max(0, std::min(MIX_MAX_VOLUME, sdlVolume));
    Mix_Volume(handle.channelId, sdlVolume);
#endif
}

// ========== Music ==========

bool AudioSystem::LoadMusic(const std::string& filePath) {
    if (!initialized_) {
        std::cerr << "AudioSystem: Cannot load music, system not initialized" << std::endl;
        return false;
    }
    
#ifdef USE_SDL_MIXER
    // Free existing music if different
    if (currentMusicPath_ != filePath) {
        if (currentMusic_) {
            Mix_FreeMusic(static_cast<Mix_Music*>(currentMusic_));
            currentMusic_ = nullptr;
        }
        
        // Load new music
        Mix_Music* music = Mix_LoadMUS(filePath.c_str());
        if (!music) {
            std::cerr << "AudioSystem: Failed to load music '" << filePath << "' - " << Mix_GetError() << std::endl;
            return false;
        }
        
        currentMusic_ = music;
        currentMusicPath_ = filePath;
        // std::cout << "AudioSystem: Loaded music '" << filePath << "'" << std::endl;
    }
    
    return true;
#else
    return false;
#endif
}

void AudioSystem::PlayMusic(const std::string& filePath, bool loop, float fadeInMs) {
    if (!initialized_) {
        std::cerr << "AudioSystem: Cannot play music, system not initialized" << std::endl;
        return;
    }
    
#ifdef USE_SDL_MIXER
    // Load music if not already loaded or different file
    if (!LoadMusic(filePath)) {
        return;
    }
    
    Mix_Music* music = static_cast<Mix_Music*>(currentMusic_);
    
    // Play music (-1 = infinite loop, 0 = play once)
    int loops = loop ? -1 : 0;
    int fadeMs = static_cast<int>(fadeInMs);
    
    if (fadeMs > 0) {
        if (Mix_FadeInMusic(music, loops, fadeMs) < 0) {
            std::cerr << "AudioSystem: Failed to play music - " << Mix_GetError() << std::endl;
        }
    } else {
        if (Mix_PlayMusic(music, loops) < 0) {
            std::cerr << "AudioSystem: Failed to play music - " << Mix_GetError() << std::endl;
        }
    }
    
    // Apply music volume
    float finalVolume = musicVolume_ * masterVolume_;
    int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
    Mix_VolumeMusic(sdlVolume);
#endif
}

void AudioSystem::StopMusic(float fadeOutMs) {
    if (!initialized_) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    int fadeMs = static_cast<int>(fadeOutMs);
    if (fadeMs > 0) {
        Mix_FadeOutMusic(fadeMs);
    } else {
        Mix_HaltMusic();
    }
#endif
}

void AudioSystem::PauseMusic() {
    if (!initialized_) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    Mix_PauseMusic();
#endif
}

void AudioSystem::ResumeMusic() {
    if (!initialized_) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    Mix_ResumeMusic();
#endif
}

bool AudioSystem::IsMusicPlaying() {
    if (!initialized_) {
        return false;
    }
    
#ifdef USE_SDL_MIXER
    return Mix_PlayingMusic() != 0;
#else
    return false;
#endif
}

// ========== Volume Control ==========

void AudioSystem::SetMasterVolume(float volume) {
    masterVolume_ = std::max(0.0f, std::min(1.0f, volume));
    
    // Update music volume immediately
    if (initialized_) {
#ifdef USE_SDL_MIXER
        float finalMusicVolume = musicVolume_ * masterVolume_;
        int sdlVolume = static_cast<int>(finalMusicVolume * MIX_MAX_VOLUME);
        Mix_VolumeMusic(sdlVolume);
        
        // Note: Active sound effects will keep their volume until next SetSoundVolume call
        // This is acceptable since most sounds are short-lived
#endif
    }
}

void AudioSystem::SetSFXVolume(float volume) {
    sfxVolume_ = std::max(0.0f, std::min(1.0f, volume));
    // Active sounds keep their volume until explicitly updated
}

void AudioSystem::SetMusicVolume(float volume) {
    musicVolume_ = std::max(0.0f, std::min(1.0f, volume));
    
    if (initialized_) {
#ifdef USE_SDL_MIXER
        float finalVolume = musicVolume_ * masterVolume_;
        int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
        Mix_VolumeMusic(sdlVolume);
#endif
    }
}

float AudioSystem::GetMasterVolume() {
    return masterVolume_;
}

float AudioSystem::GetSFXVolume() {
    return sfxVolume_;
}

float AudioSystem::GetMusicVolume() {
    return musicVolume_;
}

// ========== Spatial Audio ==========

void AudioSystem::SetListenerPosition(double x, double y, double z) {
    listenerX_ = x;
    listenerY_ = y;
    listenerZ_ = z;
}

void AudioSystem::GetListenerPosition(double& x, double& y, double& z) {
    x = listenerX_;
    y = listenerY_;
    z = listenerZ_;
}

void AudioSystem::SetListenerOrientation(double forwardX, double forwardY, double forwardZ,
                                        double upX, double upY, double upZ) {
    listenerForwardX_ = forwardX;
    listenerForwardY_ = forwardY;
    listenerForwardZ_ = forwardZ;
    listenerUpX_ = upX;
    listenerUpY_ = upY;
    listenerUpZ_ = upZ;
}

// ========== Configuration ==========

void AudioSystem::SetMaxAudioDistance(double distance) {
    maxAudioDistance_ = std::max(0.1, distance);
}

void AudioSystem::SetReferenceDistance(double distance) {
    referenceDistance_ = std::max(0.1, distance);
}

void AudioSystem::SetRolloffFactor(double factor) {
    rolloffFactor_ = std::max(0.0, factor);
}

// ========== Helper Methods ==========

float AudioSystem::CalculateDistanceAttenuation(double x, double y, double z) {
    // Calculate distance from listener
    double dx = x - listenerX_;
    double dy = y - listenerY_;
    double dz = z - listenerZ_;
    double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Full volume within reference distance
    if (distance < referenceDistance_) {
        return 1.0f;
    }
    
    // Inaudible beyond max distance
    if (distance > maxAudioDistance_) {
        return 0.0f;
    }
    
    // Inverse distance attenuation
    // attenuation = referenceDistance / (referenceDistance + rolloffFactor * (distance - referenceDistance))
    double attenuation = referenceDistance_ / 
                        (referenceDistance_ + rolloffFactor_ * (distance - referenceDistance_));
    
    return static_cast<float>(std::max(0.0, std::min(1.0, attenuation)));
}

int AudioSystem::CalculateStereoPan(double x, double y, double z) {
    // Calculate horizontal angle (ignoring y/vertical for simplicity)
    (void)y; // Unused for 2D panning
    double dx = x - listenerX_;
    double dz = z - listenerZ_;
    
    // Calculate angle in radians (-PI to PI)
    // atan2(dx, dz) gives angle where:
    // - 0 = forward (center)
    // - +PI/2 = right
    // - -PI/2 = left
    double angle = std::atan2(dx, dz);
    
    // Convert to pan value (0 = left, 127 = center, 255 = right)
    // Note: SDL_mixer's Mix_SetPanning is weird - we'll correct in PlaySound3D
    double normalized = (angle / M_PI); // -1.0 to +1.0
    int pan = static_cast<int>(127.0 + normalized * 127.0);
    
    // Clamp to valid range
    pan = std::max(0, std::min(255, pan));
    
    return pan;
}

void AudioSystem::ApplyVolumeToChannel(int channel, float baseVolume) {
    if (!initialized_ || channel < 0) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    float finalVolume = baseVolume * sfxVolume_ * masterVolume_;
    int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
    sdlVolume = std::max(0, std::min(MIX_MAX_VOLUME, sdlVolume));
    Mix_Volume(channel, sdlVolume);
#endif
}

void AudioSystem::ApplyPanningToChannel(int channel, int pan) {
    if (!initialized_ || channel < 0) {
        return;
    }
    
#ifdef USE_SDL_MIXER
    // SDL_mixer SetPanning: (left, right) where 0 = silent, 255 = full
    // Our pan: 0 = left, 127 = center, 255 = right
    int left = 255 - pan;
    int right = pan;
    Mix_SetPanning(channel, left, right);
#endif
}

