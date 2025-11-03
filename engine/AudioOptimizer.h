#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace Nova {

// Audio compression formats
enum class AudioFormat {
    Unknown,
    WAV_PCM,        // Uncompressed PCM
    WAV_ADPCM,      // Compressed WAV
    MP3,            // MPEG Layer-3
    OGG_Vorbis,     // Ogg Vorbis
    FLAC,           // Free Lossless Audio Codec
    AAC,            // Advanced Audio Codec
    Opus            // Modern low-latency codec
};

// Audio quality settings
enum class AudioQuality {
    Low,       // 64 kbps, mono, 22kHz
    Medium,    // 128 kbps, stereo, 44.1kHz
    High,      // 192 kbps, stereo, 44.1kHz
    Lossless   // Original quality
};

// Audio streaming settings
struct AudioStreamConfig {
    bool enabled = true;
    size_t bufferSize = 4096;       // Samples per buffer
    int numBuffers = 4;             // Number of buffers
    float prebufferTime = 0.5f;     // Seconds to prebuffer
    bool loopStream = false;
    int priority = 50;              // 0-100
};

// Audio metadata
struct AudioMetadata {
    std::string path;
    AudioFormat format = AudioFormat::Unknown;
    int sampleRate = 0;
    int channels = 0;
    int bitDepth = 0;
    int bitrate = 0;
    float duration = 0.0f;
    size_t memorySize = 0;
    size_t compressedSize = 0;
    bool isStreaming = false;
    bool isLooping = false;
};

// 3D audio settings
struct Audio3DConfig {
    bool enabled = true;
    float dopplerFactor = 1.0f;
    float speedOfSound = 343.3f;
    float referenceDistance = 1.0f;
    float maxDistance = 1000.0f;
    float rolloffFactor = 1.0f;
    std::string attenuationModel = "inverse";  // "none", "inverse", "linear", "exponential"
};

class AudioOptimizer {
public:
    static AudioOptimizer& GetInstance() {
        static AudioOptimizer instance;
        return instance;
    }

    // === AUDIO COMPRESSION ===
    bool CompressAudio(const std::string& inputPath, const std::string& outputPath,
                      AudioFormat targetFormat, AudioQuality quality = AudioQuality::High);
    bool ConvertFormat(const std::string& inputPath, const std::string& outputPath,
                      AudioFormat sourceFormat, AudioFormat targetFormat);
    AudioFormat SelectOptimalFormat(bool isMusic, bool needsLowLatency, bool needsSmallSize);
    size_t EstimateCompressedSize(const AudioMetadata& metadata, AudioFormat format, 
                                  AudioQuality quality);
    
    // === SAMPLE RATE CONVERSION ===
    bool ResampleAudio(const std::string& inputPath, const std::string& outputPath,
                      int targetSampleRate, bool highQuality = true);
    int SelectOptimalSampleRate(AudioQuality quality);
    
    // === CHANNEL CONVERSION ===
    bool ConvertChannels(const std::string& inputPath, const std::string& outputPath,
                        int targetChannels);  // 1=mono, 2=stereo
    bool MonoToStereo(const std::string& inputPath, const std::string& outputPath);
    bool StereoToMono(const std::string& inputPath, const std::string& outputPath);
    
    // === AUDIO STREAMING ===
    void EnableStreaming(const std::string& audioPath, bool enable = true);
    bool IsStreaming(const std::string& audioPath) const;
    void SetStreamConfig(const std::string& audioPath, const AudioStreamConfig& config);
    AudioStreamConfig GetStreamConfig(const std::string& audioPath) const;
    void UpdateStreamingPriorities(const std::vector<std::string>& activeSounds);
    size_t CalculateStreamingMemory(const AudioStreamConfig& config, int sampleRate, int channels) const;
    
    // === AUDIO EFFECTS ===
    bool NormalizeAudio(const std::string& inputPath, const std::string& outputPath,
                       float targetLevel = -3.0f);  // dB
    bool ApplyFade(const std::string& inputPath, const std::string& outputPath,
                  float fadeInTime, float fadeOutTime);
    bool ApplyCompression(const std::string& inputPath, const std::string& outputPath,
                         float threshold, float ratio);
    bool ApplyEQ(const std::string& inputPath, const std::string& outputPath,
                const std::vector<float>& bandGains);  // Per-band gain in dB
    
    // === 3D AUDIO ===
    void Set3DConfig(const Audio3DConfig& config);
    Audio3DConfig Get3DConfig() const { return audio3DConfig_; }
    float CalculateAttenuation(float distance) const;
    void UpdateListenerPosition(float x, float y, float z, float vx, float vy, float vz);
    void Update3DSource(const std::string& audioPath, float x, float y, float z,
                       float vx, float vy, float vz);
    
    // === AUDIO METADATA ===
    AudioMetadata GetMetadata(const std::string& audioPath);
    void CacheMetadata(const std::string& audioPath, const AudioMetadata& metadata);
    size_t AnalyzeMemoryUsage(const std::string& audioPath);
    std::vector<std::string> FindUnoptimizedAudio(size_t minSize = 1024 * 1024);
    
    // === QUALITY PRESETS ===
    void SetQualityPreset(const std::string& preset);  // "low", "medium", "high", "lossless"
    AudioQuality GetCurrentQuality() const { return currentQuality_; }
    void ApplyQualityToAll(AudioQuality quality);
    
    // === BATCH OPERATIONS ===
    void OptimizeDirectory(const std::string& directory, bool recursive = true);
    void CompressDirectory(const std::string& directory, AudioFormat format,
                          AudioQuality quality, bool recursive = true);
    int BatchConvert(const std::vector<std::string>& audioFiles, AudioFormat targetFormat);
    void NormalizeDirectory(const std::string& directory, float targetLevel, bool recursive = true);
    
    // === AUDIO POOLING ===
    void PreloadAudio(const std::string& audioPath);
    void UnloadAudio(const std::string& audioPath);
    void PreloadCategory(const std::string& category);  // "sfx", "music", "voice", "ambient"
    void UnloadCategory(const std::string& category);
    void SetCategoryVolume(const std::string& category, float volume);
    
    // === MEMORY MANAGEMENT ===
    size_t GetTotalAudioMemory() const;
    size_t GetStreamingAudioMemory() const;
    void SetMemoryBudget(size_t maxBytes);
    bool IsWithinMemoryBudget() const;
    void UnloadLeastRecentlyUsed(size_t targetBytes);
    
    // === DIAGNOSTICS ===
    void DumpAudioReport(const std::string& outputPath);
    int GetLoadedAudioCount() const;
    int GetStreamingAudioCount() const;
    void ClearCache();
    std::string GetOptimizationReport() const;

private:
    AudioOptimizer() = default;
    ~AudioOptimizer() = default;
    
    AudioOptimizer(const AudioOptimizer&) = delete;
    AudioOptimizer& operator=(const AudioOptimizer&) = delete;

    // Internal helpers
    bool LoadAudioData(const std::string& path, std::vector<float>& samples,
                      int& sampleRate, int& channels);
    bool SaveAudioData(const std::string& path, const std::vector<float>& samples,
                      int sampleRate, int channels, AudioFormat format);
    void ResampleBuffer(const std::vector<float>& input, std::vector<float>& output,
                       int inputRate, int outputRate, int channels);
    void ConvertChannelLayout(const std::vector<float>& input, std::vector<float>& output,
                             int inputChannels, int outputChannels);

    std::unordered_map<std::string, AudioMetadata> metadataCache_;
    std::unordered_map<std::string, AudioStreamConfig> streamConfigs_;
    std::unordered_map<std::string, bool> streamingAudio_;
    std::unordered_map<std::string, std::string> audioCategories_;
    std::unordered_map<std::string, float> categoryVolumes_;
    
    Audio3DConfig audio3DConfig_;
    AudioQuality currentQuality_ = AudioQuality::High;
    size_t memoryBudget_ = 128 * 1024 * 1024;  // 128 MB
    
    struct ListenerState {
        float x = 0, y = 0, z = 0;
        float vx = 0, vy = 0, vz = 0;
    } listenerState_;
};

} // namespace Nova
