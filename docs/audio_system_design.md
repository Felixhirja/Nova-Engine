# Audio System Design Document

## Overview

Star Engine's audio system provides comprehensive audio playback capabilities including:
- **Sound effects (SFX)** - One-shot and looping sounds for gameplay events
- **Background music** - Streaming music with crossfading and playlists
- **Spatial audio** - 3D positioned sounds with distance attenuation
- **Volume control** - Independent volume controls for Master, SFX, Music, and Alarms
- **Resource management** - Efficient loading and caching of audio assets

---

## Library Selection: SDL_mixer

**Chosen Library**: `SDL_mixer` (SDL2_mixer / SDL3_mixer)

### Why SDL_mixer?

✅ **Pros**:
- Already using SDL for windowing/input - no new dependencies
- Simple API - easy to integrate and maintain
- Cross-platform (Windows, Linux, macOS)
- Supports multiple formats (WAV, OGG, MP3)
- Built-in mixing with multiple channels
- Music streaming (doesn't load entire file into memory)
- Active development and good documentation
- Lightweight (~200KB)

❌ **vs OpenAL**:
- OpenAL has better 3D audio positioning (full HRTF, reverb)
- OpenAL is more complex to set up
- Would add another dependency to the project
- SDL_mixer's manual panning is sufficient for space game

### SDL_mixer Capabilities

| Feature | SDL_mixer Support | Notes |
|---------|-------------------|-------|
| Sound Effects | ✅ Multiple channels | Up to 32 channels by default |
| Music | ✅ Streaming | One music track at a time |
| Volume Control | ✅ Per-channel + Master | 0-128 scale |
| 3D Audio | ⚠️ Manual stereo panning | Calculate panning based on position |
| Distance Attenuation | ⚠️ Manual volume scaling | Calculate volume based on distance |
| File Formats | ✅ WAV, OGG, MP3, FLAC | OGG recommended for music |
| Looping | ✅ Full support | Infinite or count-based loops |

---

## Architecture

### Class Structure

```
AudioSystem (Core Manager)
├── Sound effect playback (SDL_mixer chunks)
├── Music playback (SDL_mixer music)
├── Volume management (Master, SFX, Music, Alarms)
├── Spatial audio calculations (distance, panning)
└── Resource management (loading, caching)

AudioFeedbackSystem (Game Integration)
├── Listens to FeedbackEvents
├── Calls AudioSystem for playback
└── Manages gameplay-specific audio logic

Settings Menu
├── Volume sliders
└── Audio preferences
```

### File Organization

**New Files**:
- `src/AudioSystem.h` - Core audio manager interface
- `src/AudioSystem.cpp` - SDL_mixer implementation
- `docs/audio_system_api.md` - API documentation
- `docs/audio_asset_guidelines.md` - Asset creation guide
- `tests/test_audio.cpp` - Unit tests

**Modified Files**:
- `src/AudioFeedbackSystem.cpp` - Replace stubs with AudioSystem calls
- `Makefile` - Add SDL_mixer linker flags
- `src/MainLoop.cpp` - Initialize AudioSystem on startup

**Assets**:
- `assets/audio/sfx/` - Sound effects (.wav or .ogg)
- `assets/audio/music/` - Background music (.ogg)

---

## AudioSystem API Design

### Core Interface

```cpp
class AudioSystem {
public:
    // Initialization
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized();
    
    // Sound Effects
    struct SoundHandle {
        int channelId;
        int clipId;
    };
    
    static int LoadSound(const std::string& filePath);
    static SoundHandle PlaySound(int clipId, bool loop = false, float volume = 1.0f);
    static SoundHandle PlaySound3D(int clipId, double x, double y, double z, 
                                    bool loop = false, float volume = 1.0f);
    static void StopSound(SoundHandle handle);
    static void StopAllSounds();
    static bool IsSoundPlaying(SoundHandle handle);
    
    // Music
    static bool LoadMusic(const std::string& filePath);
    static void PlayMusic(const std::string& filePath, bool loop = true, float fadeInMs = 0.0f);
    static void StopMusic(float fadeOutMs = 0.0f);
    static void PauseMusic();
    static void ResumeMusic();
    static bool IsMusicPlaying();
    
    // Volume Control (0.0 to 1.0)
    static void SetMasterVolume(float volume);
    static void SetSFXVolume(float volume);
    static void SetMusicVolume(float volume);
    static float GetMasterVolume();
    static float GetSFXVolume();
    static float GetMusicVolume();
    
    // Listener Position (for 3D audio)
    static void SetListenerPosition(double x, double y, double z);
    static void GetListenerPosition(double& x, double& y, double& z);
    
private:
    // Internal state
    static bool initialized_;
    static std::unordered_map<int, Mix_Chunk*> soundClips_;
    static std::unordered_map<std::string, int> soundPaths_;
    static Mix_Music* currentMusic_;
    static std::string currentMusicPath_;
    static float masterVolume_;
    static float sfxVolume_;
    static float musicVolume_;
    static double listenerX_, listenerY_, listenerZ_;
    static int nextClipId_;
    
    // Helpers
    static float CalculateDistanceAttenuation(double x, double y, double z);
    static int CalculateStereo Pan(double x, double y, double z);
    static void ApplyVolumeToChannel(int channel, float baseVolume);
};
```

---

## 3D Audio Implementation

### Distance Attenuation

```cpp
float AudioSystem::CalculateDistanceAttenuation(double x, double y, double z) {
    double dx = x - listenerX_;
    double dy = y - listenerY_;
    double dz = z - listenerZ_;
    double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    
    // Parameters
    const double referenceDistance = 1.0;   // Full volume distance
    const double maxDistance = 100.0;       // Inaudible distance
    const double rolloffFactor = 1.0;       // Attenuation curve
    
    if (distance < referenceDistance) {
        return 1.0f; // Full volume
    }
    
    // Inverse distance attenuation
    float attenuation = referenceDistance / 
                       (referenceDistance + rolloffFactor * (distance - referenceDistance));
    
    // Clamp to max distance
    if (distance > maxDistance) {
        return 0.0f;
    }
    
    return attenuation;
}
```

### Stereo Panning

```cpp
int AudioSystem::CalculateStereoPan(double x, double y, double z) {
    double dx = x - listenerX_;
    // Ignore y (up/down) and z (forward/back) for simple left-right panning
    
    // Calculate angle in radians
    double angle = std::atan2(dx, 0.0); // Left = negative, Right = positive
    
    // Convert to SDL_mixer panning (0 = left, 127 = center, 255 = right)
    int pan = static_cast<int>(127.0 + (angle / M_PI) * 127.0);
    pan = std::max(0, std::min(255, pan));
    
    return pan;
}
```

---

## Volume Management

### Volume Hierarchy

```
Master Volume (0.0 - 1.0)
├── SFX Volume (0.0 - 1.0)
│   ├── UI Sounds
│   ├── Weapon Sounds
│   ├── Explosion Sounds
│   └── Alarms (may have separate control)
└── Music Volume (0.0 - 1.0)
    └── Background music tracks
```

### Applying Volumes

```cpp
void AudioSystem::ApplyVolumeToChannel(int channel, float baseVolume) {
    float finalVolume = baseVolume * sfxVolume_ * masterVolume_;
    int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
    Mix_Volume(channel, sdlVolume);
}

void AudioSystem::SetMusicVolume(float volume) {
    musicVolume_ = std::max(0.0f, std::min(1.0f, volume));
    float finalVolume = musicVolume_ * masterVolume_;
    int sdlVolume = static_cast<int>(finalVolume * MIX_MAX_VOLUME);
    Mix_VolumeMusic(sdlVolume);
}
```

---

## Resource Management

### Loading Strategy

**Sound Effects**:
- Loaded into memory as `Mix_Chunk*`
- Cached by file path (don't reload duplicates)
- Kept in memory for fast playback
- Use `.wav` for small sounds (< 1MB)
- Use `.ogg` for larger sounds (compressed)

**Music**:
- Streamed from disk as `Mix_Music*`
- Only one track in memory at a time
- Use `.ogg` format (good compression, looping support)
- Typical size: 3-5MB per minute

### Memory Budget

| Asset Type | Per Asset | Total Budget | Notes |
|------------|-----------|--------------|-------|
| Sound Effects | 100KB avg | 10-20MB | ~100-200 sounds |
| Music | Streaming | 5MB | One track at a time |
| **Total** | - | **15-25MB** | Very reasonable |

### Lazy Loading

```cpp
int AudioSystem::LoadSound(const std::string& filePath) {
    // Check if already loaded
    auto it = soundPaths_.find(filePath);
    if (it != soundPaths_.end()) {
        return it->second; // Return existing ID
    }
    
    // Load new sound
    Mix_Chunk* chunk = Mix_LoadWAV(filePath.c_str());
    if (!chunk) {
        std::cerr << "Failed to load sound: " << filePath 
                  << " - " << Mix_GetError() << std::endl;
        return -1;
    }
    
    int clipId = nextClipId_++;
    soundClips_[clipId] = chunk;
    soundPaths_[filePath] = clipId;
    return clipId;
}
```

---

## Integration with AudioFeedbackSystem

### Before (Stub)

```cpp
int AudioFeedbackSystem::PlaySound3D(const std::string& clipName, 
                                    double x, double y, double z, bool loop) {
    // TODO: Actually play audio
    std::cout << "Playing sound: " << clipName << std::endl;
    return -1;
}
```

### After (Real Implementation)

```cpp
int AudioFeedbackSystem::PlaySound3D(const std::string& clipName, 
                                    double x, double y, double z, bool loop) {
    auto it = clips_.find(clipName);
    if (it == clips_.end()) {
        return -1;
    }
    
    // Load sound if not already cached
    if (it->second.clipId == -1) {
        it->second.clipId = AudioSystem::LoadSound(it->second.filePath);
    }
    
    // Play with 3D positioning
    auto handle = AudioSystem::PlaySound3D(it->second.clipId, x, y, z, 
                                          loop, it->second.volume);
    
    // Track active playback
    int soundId = nextSoundId_++;
    activePlaybacks_[soundId] = handle;
    return soundId;
}
```

---

## Settings Menu Integration

### Audio Settings Screen

```cpp
class AudioSettingsMenu : public MenuSystem {
public:
    AudioSettingsMenu();
    
    void Update(float deltaTime) override;
    void HandleKeyPress(int key, int action, int mods) override;
    void HandleMouseMove(int x, int y) override;
    
    // Volume adjustments
    void IncreaseMasterVolume();
    void DecreaseMasterVolume();
    void IncreaseSFXVolume();
    void DecreaseSFXVolume();
    void IncreaseMusicVolume();
    void DecreaseMusicVolume();
    
private:
    void RefreshVolumeDisplay();
};
```

### Volume Sliders

```
┌─────────────────────────────────┐
│       AUDIO SETTINGS            │
├─────────────────────────────────┤
│                                 │
│  Master Volume:  [████████░░] 80%│
│  SFX Volume:     [██████████] 100%│
│  Music Volume:   [█████░░░░░] 50%│
│  Alarm Volume:   [███████░░░] 70%│
│                                 │
│  [ Test SFX ]  [ Test Music ]  │
│                                 │
│  [ Back to Main Menu ]          │
└─────────────────────────────────┘
```

---

## Build System Changes

### Makefile Modifications

```makefile
# SDL_mixer detection
SDL_MIXER_CFLAGS := $(shell pkg-config --cflags SDL2_mixer 2>/dev/null)
SDL_MIXER_LIBS := $(shell pkg-config --libs SDL2_mixer 2>/dev/null)

# Fallback if pkg-config fails
ifeq ($(SDL_MIXER_LIBS),)
SDL_MIXER_CFLAGS := -IC:/msys64/mingw64/include
SDL_MIXER_LIBS := -LC:/msys64/mingw64/lib -lSDL2_mixer
endif

ifneq ($(SDL_MIXER_LIBS),)
    CXXFLAGS += $(SDL_MIXER_CFLAGS) -DUSE_SDL_MIXER
    LDLIBS += $(SDL_MIXER_LIBS)
$(info SDL_mixer detected: building with audio support)
else
$(info SDL_mixer not found; building without audio)
endif
```

### Installation (Windows/MSYS2)

```bash
# Install SDL2_mixer
pacman -S mingw-w64-x86_64-SDL2_mixer

# Or for SDL3
pacman -S mingw-w64-x86_64-SDL3_mixer
```

---

## Audio Asset Guidelines

### Sound Effects

**Format**: `.wav` (uncompressed) or `.ogg` (compressed)  
**Sample Rate**: 44100 Hz or 48000 Hz  
**Bit Depth**: 16-bit  
**Channels**: Mono (for 3D sounds) or Stereo (for UI sounds)

**Categories**:
- **UI**: Button clicks, menu navigation (short, clean)
- **Weapons**: Lasers, explosions, impacts (punchy, satisfying)
- **Shields**: Energy hums, impacts, recharge (sci-fi, futuristic)
- **Alarms**: Warnings, critical alerts (attention-grabbing, loopable)
- **Engines**: Thrusters, maneuvers (continuous, loopable)

### Background Music

**Format**: `.ogg` (Vorbis compression)  
**Sample Rate**: 44100 Hz  
**Bit Depth**: 16-bit (variable bitrate)  
**Channels**: Stereo  
**Length**: 2-5 minutes (seamless looping)

**Moods**:
- **Main Menu**: Epic, welcoming, atmospheric
- **Space Flight**: Ambient, calm, exploration
- **Combat**: Intense, rhythmic, adrenaline
- **Victory**: Triumphant, uplifting
- **Defeat**: Somber, reflective

### File Naming Convention

```
assets/audio/
├── sfx/
│   ├── ui/
│   │   ├── button_click.wav
│   │   ├── menu_open.wav
│   │   └── menu_close.wav
│   ├── weapons/
│   │   ├── laser_fire_01.wav
│   │   ├── explosion_small_01.ogg
│   │   └── explosion_large_01.ogg
│   ├── shields/
│   │   ├── shield_hit_01.wav
│   │   ├── shield_depleted.wav
│   │   └── shield_recharge_loop.ogg
│   └── alarms/
│       ├── warning_loop.ogg
│       └── critical_loop.ogg
└── music/
    ├── main_menu.ogg
    ├── space_exploration.ogg
    ├── combat_intense.ogg
    └── victory.ogg
```

---

## Testing Strategy

### Unit Tests (`tests/test_audio.cpp`)

```cpp
void TestAudioInitialization() {
    ASSERT_TRUE(AudioSystem::Initialize());
    ASSERT_TRUE(AudioSystem::IsInitialized());
    AudioSystem::Shutdown();
    ASSERT_FALSE(AudioSystem::IsInitialized());
}

void TestSoundLoading() {
    AudioSystem::Initialize();
    int clipId = AudioSystem::LoadSound("assets/audio/test.wav");
    ASSERT_TRUE(clipId >= 0);
    AudioSystem::Shutdown();
}

void TestVolumeControls() {
    AudioSystem::Initialize();
    AudioSystem::SetMasterVolume(0.5f);
    ASSERT_NEAR(AudioSystem::GetMasterVolume(), 0.5f, 0.01f);
    AudioSystem::SetSFXVolume(0.8f);
    ASSERT_NEAR(AudioSystem::GetSFXVolume(), 0.8f, 0.01f);
    AudioSystem::Shutdown();
}

void TestSpatialAudio() {
    AudioSystem::Initialize();
    AudioSystem::SetListenerPosition(0.0, 0.0, 0.0);
    
    int clipId = AudioSystem::LoadSound("assets/audio/test.wav");
    auto handle = AudioSystem::PlaySound3D(clipId, 10.0, 0.0, 0.0);
    ASSERT_TRUE(AudioSystem::IsSoundPlaying(handle));
    
    AudioSystem::StopSound(handle);
    AudioSystem::Shutdown();
}
```

### Manual Testing Checklist

- [ ] Sound effects play correctly
- [ ] 3D positioning: sounds pan left/right based on position
- [ ] Distance attenuation: sounds fade with distance
- [ ] Music plays and loops seamlessly
- [ ] Volume controls affect all sounds
- [ ] Multiple sounds can play simultaneously
- [ ] Stopping sounds works correctly
- [ ] No memory leaks after many play/stop cycles
- [ ] Graceful handling of missing audio files

---

## Performance Considerations

### Optimization Tips

1. **Preload frequently used sounds** - Load menu sounds, common SFX on startup
2. **Limit active channels** - SDL_mixer default is 8, increase to 32 if needed
3. **Use compressed formats** - OGG for large files, WAV for small sounds
4. **Avoid per-frame updates** - Only update 3D audio when listener moves significantly
5. **Pool sound instances** - Reuse Mix_Chunks instead of reloading

### Typical Performance

| Operation | Time | Notes |
|-----------|------|-------|
| Load WAV (100KB) | <1ms | Cached after first load |
| Play sound | <0.1ms | Very fast, hardware accelerated |
| 3D calculations | <0.01ms | Simple math, negligible |
| Update listener | <0.05ms | Only when camera moves |
| Music streaming | 0ms | Handled by SDL_mixer thread |

**Expected CPU usage**: <1% for normal gameplay audio

---

## Troubleshooting

### Common Issues

**Sound doesn't play**:
- Check if SDL_mixer initialized: `AudioSystem::IsInitialized()`
- Verify file path is correct
- Check if file format is supported
- Ensure volume isn't zero

**Crackling/popping**:
- Increase audio buffer size: `Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048)`
- Check CPU usage (might be overloaded)

**Music doesn't loop**:
- Ensure loop parameter is `true` or `-1`
- Check if OGG file has proper loop points

**Volume too quiet**:
- Check master volume, SFX volume, and per-sound volume
- Ensure audio files aren't normalized too low

---

## Future Enhancements

### Phase 2 Features
- **Dynamic music** - Crossfade between combat/exploration tracks
- **Audio occlusion** - Sounds muffled when behind objects
- **Doppler effect** - Pitch shifts based on velocity
- **Reverb zones** - Echo effects in large spaces
- **Voice lines** - NPC chatter, ship computer warnings

### Phase 3 Features
- **OpenAL integration** - Full 3D audio with HRTF
- **Audio compression** - Runtime decompression for larger games
- **Audio scripting** - Lua/Python bindings for modding
- **Procedural audio** - Generate sounds algorithmically

---

## Summary

**Implementation Time Estimate**: 8-12 hours
- 2 hours: AudioSystem core class
- 2 hours: SDL_mixer integration
- 1 hour: AudioFeedbackSystem connection
- 1 hour: Music system
- 2 hours: Settings menu UI
- 1 hour: Testing
- 1 hour: Documentation
- 2 hours: Asset creation/testing

**Priority Order**:
1. ✅ Core AudioSystem class (2 hours)
2. ✅ SDL_mixer integration (2 hours)
3. ✅ Connect to AudioFeedbackSystem (1 hour)
4. ⏸️ Music playback (1 hour)
5. ⏸️ Settings menu (2 hours)
6. ⏸️ Spatial audio (1 hour)
7. ⏸️ Testing & polish (2 hours)

**Next Step**: Create `src/AudioSystem.h` with core interface! 🎵
