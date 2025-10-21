#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Forward declarations for SDL_mixer types
#ifdef USE_SDL_MIXER
struct Mix_Chunk;
// Mix_Music is opaque - we'll just use void* in the header
#endif

/**
 * @brief Core audio system for Nova Engine
 * 
 * Provides comprehensive audio playback using SDL_mixer:
 * - Sound effects (2D and 3D positioned)
 * - Background music with streaming
 * - Independent volume controls (Master, SFX, Music)
 * - Spatial audio with distance attenuation and stereo panning
 * - Resource management with caching
 * 
 * All methods are static - AudioSystem is a singleton manager.
 */
class AudioSystem {
public:
    // ========== Initialization ==========
    
    /**
     * Initialize the audio system
     * @return true if initialization succeeded, false otherwise
     */
    static bool Initialize();
    
    /**
     * Shutdown the audio system and free all resources
     */
    static void Shutdown();
    
    /**
     * Check if audio system is initialized
     * @return true if initialized, false otherwise
     */
    static bool IsInitialized();
    
    // ========== Sound Effects ==========
    
    /**
     * Handle to a playing sound instance
     */
    struct SoundHandle {
        int channelId;  // SDL_mixer channel ID (-1 if invalid)
        int clipId;     // Internal clip ID
        
        bool IsValid() const { return channelId >= 0; }
    };
    
    /**
     * Load a sound effect from file
     * @param filePath Path to .wav or .ogg file
     * @return Clip ID (>= 0) on success, -1 on failure
     */
    static int LoadSound(const std::string& filePath);
    
    /**
     * Play a sound effect (2D, no spatial positioning)
     * @param clipId Clip ID from LoadSound()
     * @param loop If true, loop indefinitely (-1 loops). If false, play once (0 loops).
     * @param volume Volume multiplier (0.0 to 1.0), multiplied with SFX and Master volumes
     * @return SoundHandle to control the playing sound
     */
    static SoundHandle PlaySound(int clipId, bool loop = false, float volume = 1.0f);
    
    /**
     * Play a sound effect with 3D positioning
     * @param clipId Clip ID from LoadSound()
     * @param x X position in world space
     * @param y Y position in world space
     * @param z Z position in world space
     * @param loop If true, loop indefinitely. If false, play once.
     * @param volume Base volume (0.0 to 1.0), before distance attenuation
     * @return SoundHandle to control the playing sound
     */
    static SoundHandle PlaySound3D(int clipId, double x, double y, double z, 
                                    bool loop = false, float volume = 1.0f);
    
    /**
     * Stop a playing sound
     * @param handle SoundHandle returned from PlaySound() or PlaySound3D()
     */
    static void StopSound(SoundHandle handle);
    
    /**
     * Stop all currently playing sounds
     */
    static void StopAllSounds();
    
    /**
     * Check if a sound is currently playing
     * @param handle SoundHandle to check
     * @return true if sound is still playing, false otherwise
     */
    static bool IsSoundPlaying(SoundHandle handle);
    
    /**
     * Update volume for a playing sound (useful for 3D repositioning)
     * @param handle SoundHandle to update
     * @param volume New volume (0.0 to 1.0)
     */
    static void SetSoundVolume(SoundHandle handle, float volume);
    
    // ========== Music ==========
    
    /**
     * Load music from file (streaming, not loaded into memory)
     * @param filePath Path to .ogg, .mp3, or .wav file (OGG recommended for music)
     * @return true if loaded successfully, false otherwise
     */
    static bool LoadMusic(const std::string& filePath);
    
    /**
     * Play music track
     * @param filePath Path to music file (will be loaded if not already)
     * @param loop If true, loop indefinitely. If false, play once.
     * @param fadeInMs Fade in duration in milliseconds (0 = no fade)
     */
    static void PlayMusic(const std::string& filePath, bool loop = true, float fadeInMs = 0.0f);
    
    /**
     * Stop currently playing music
     * @param fadeOutMs Fade out duration in milliseconds (0 = stop immediately)
     */
    static void StopMusic(float fadeOutMs = 0.0f);
    
    /**
     * Pause currently playing music
     */
    static void PauseMusic();
    
    /**
     * Resume paused music
     */
    static void ResumeMusic();
    
    /**
     * Check if music is currently playing
     * @return true if music is playing, false otherwise
     */
    static bool IsMusicPlaying();
    
    // ========== Volume Control ==========
    
    /**
     * Set master volume (affects all audio)
     * @param volume Volume level (0.0 = silent, 1.0 = full)
     */
    static void SetMasterVolume(float volume);
    
    /**
     * Set sound effects volume
     * @param volume Volume level (0.0 = silent, 1.0 = full)
     */
    static void SetSFXVolume(float volume);
    
    /**
     * Set music volume
     * @param volume Volume level (0.0 = silent, 1.0 = full)
     */
    static void SetMusicVolume(float volume);
    
    /**
     * Get current master volume
     * @return Volume level (0.0 to 1.0)
     */
    static float GetMasterVolume();
    
    /**
     * Get current SFX volume
     * @return Volume level (0.0 to 1.0)
     */
    static float GetSFXVolume();
    
    /**
     * Get current music volume
     * @return Volume level (0.0 to 1.0)
     */
    static float GetMusicVolume();
    
    // ========== Spatial Audio (3D Positioning) ==========
    
    /**
     * Set listener position for 3D audio calculations
     * @param x X position in world space
     * @param y Y position in world space
     * @param z Z position in world space
     */
    static void SetListenerPosition(double x, double y, double z);
    
    /**
     * Get current listener position
     * @param x Output: X position
     * @param y Output: Y position
     * @param z Output: Z position
     */
    static void GetListenerPosition(double& x, double& y, double& z);
    
    /**
     * Set listener orientation (for future advanced 3D audio)
     * Currently not used by stereo panning, but provided for future OpenAL integration
     * @param forwardX Forward vector X
     * @param forwardY Forward vector Y
     * @param forwardZ Forward vector Z
     * @param upX Up vector X
     * @param upY Up vector Y
     * @param upZ Up vector Z
     */
    static void SetListenerOrientation(double forwardX, double forwardY, double forwardZ,
                                      double upX, double upY, double upZ);
    
    // ========== Configuration ==========
    
    /**
     * Set maximum distance for 3D audio (sounds beyond this are inaudible)
     * @param distance Maximum audible distance (default: 100.0)
     */
    static void SetMaxAudioDistance(double distance);
    
    /**
     * Set reference distance for 3D audio (full volume within this distance)
     * @param distance Reference distance (default: 1.0)
     */
    static void SetReferenceDistance(double distance);
    
    /**
     * Set rolloff factor for distance attenuation (higher = faster falloff)
     * @param factor Rolloff factor (default: 1.0)
     */
    static void SetRolloffFactor(double factor);

private:
    // ========== Internal State ==========
    
    static bool initialized_;
    static int nextClipId_;
    
    // Sound clip storage (Mix_Chunk*)
#ifdef USE_SDL_MIXER
    static std::unordered_map<int, Mix_Chunk*> soundClips_;
#endif
    static std::unordered_map<std::string, int> soundPaths_; // Path -> clipId lookup
    
    // Music storage
#ifdef USE_SDL_MIXER
    static void* currentMusic_; // Mix_Music* (opaque type)
#endif
    static std::string currentMusicPath_;
    
    // Volume settings (0.0 to 1.0)
    static float masterVolume_;
    static float sfxVolume_;
    static float musicVolume_;
    
    // Listener position and orientation
    static double listenerX_, listenerY_, listenerZ_;
    static double listenerForwardX_, listenerForwardY_, listenerForwardZ_;
    static double listenerUpX_, listenerUpY_, listenerUpZ_;
    
    // 3D audio parameters
    static double maxAudioDistance_;    // Beyond this, sound is inaudible
    static double referenceDistance_;   // Within this, sound is full volume
    static double rolloffFactor_;       // Attenuation curve steepness
    
    // ========== Helper Methods ==========
    
    /**
     * Calculate distance attenuation based on listener position
     * @param x Sound X position
     * @param y Sound Y position
     * @param z Sound Z position
     * @return Attenuation multiplier (0.0 to 1.0)
     */
    static float CalculateDistanceAttenuation(double x, double y, double z);
    
    /**
     * Calculate stereo panning based on horizontal position relative to listener
     * @param x Sound X position
     * @param y Sound Y position
     * @param z Sound Z position
     * @return Pan value (0 = left, 127 = center, 255 = right)
     */
    static int CalculateStereoPan(double x, double y, double z);
    
    /**
     * Apply volume to a specific channel, considering SFX and Master volumes
     * @param channel SDL_mixer channel ID
     * @param baseVolume Base volume before volume hierarchy
     */
    static void ApplyVolumeToChannel(int channel, float baseVolume);
    
    /**
     * Apply panning to a specific channel
     * @param channel SDL_mixer channel ID
     * @param pan Pan value (0 = left, 127 = center, 255 = right)
     */
    static void ApplyPanningToChannel(int channel, int pan);
};

