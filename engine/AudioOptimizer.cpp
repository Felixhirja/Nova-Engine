#include "AudioOptimizer.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace Nova {

// === AUDIO COMPRESSION ===

bool AudioOptimizer::CompressAudio(const std::string& inputPath, const std::string& outputPath,
                                   AudioFormat targetFormat, AudioQuality quality) {
    std::cout << "Compressing audio: " << inputPath << " -> " << outputPath << "\n";
    std::cout << "  Format: " << static_cast<int>(targetFormat) 
              << ", Quality: " << static_cast<int>(quality) << "\n";
    
    // Real implementation would use libraries like:
    // - libmp3lame for MP3
    // - libvorbis for OGG
    // - libopus for Opus
    // - libflac for FLAC
    
    AudioMetadata meta;
    meta.path = outputPath;
    meta.format = targetFormat;
    metadataCache_[outputPath] = meta;
    
    return true;
}

bool AudioOptimizer::ConvertFormat(const std::string& inputPath, const std::string& outputPath,
                                   AudioFormat sourceFormat, AudioFormat targetFormat) {
    std::cout << "Converting audio format: " << inputPath << "\n";
    std::cout << "  " << static_cast<int>(sourceFormat) << " -> " 
              << static_cast<int>(targetFormat) << "\n";
    return true;
}

AudioFormat AudioOptimizer::SelectOptimalFormat(bool isMusic, bool needsLowLatency, bool needsSmallSize) {
    if (needsLowLatency) {
        return AudioFormat::Opus;  // Best for low latency
    }
    
    if (isMusic) {
        if (needsSmallSize) {
            return AudioFormat::OGG_Vorbis;  // Good quality at small size
        } else {
            return AudioFormat::FLAC;  // Lossless for music
        }
    }
    
    // Sound effects
    if (needsSmallSize) {
        return AudioFormat::MP3;
    }
    
    return AudioFormat::WAV_PCM;  // Uncompressed for best quality
}

size_t AudioOptimizer::EstimateCompressedSize(const AudioMetadata& metadata, AudioFormat format,
                                              AudioQuality quality) {
    if (format == AudioFormat::WAV_PCM) {
        return metadata.sampleRate * metadata.channels * (metadata.bitDepth / 8) * metadata.duration;
    }
    
    int bitrate = 0;
    switch (quality) {
        case AudioQuality::Low: bitrate = 64000; break;
        case AudioQuality::Medium: bitrate = 128000; break;
        case AudioQuality::High: bitrate = 192000; break;
        case AudioQuality::Lossless: return metadata.memorySize;
    }
    
    return (bitrate / 8) * metadata.duration;
}

// === SAMPLE RATE CONVERSION ===

bool AudioOptimizer::ResampleAudio(const std::string& inputPath, const std::string& outputPath,
                                   int targetSampleRate, bool highQuality) {
    std::cout << "Resampling audio: " << inputPath << " -> " << targetSampleRate << "Hz\n";
    std::cout << "  Quality: " << (highQuality ? "High" : "Standard") << "\n";
    
    // Real implementation would use libsamplerate or similar
    
    return true;
}

int AudioOptimizer::SelectOptimalSampleRate(AudioQuality quality) {
    switch (quality) {
        case AudioQuality::Low: return 22050;
        case AudioQuality::Medium: return 44100;
        case AudioQuality::High: return 44100;
        case AudioQuality::Lossless: return 48000;
    }
    return 44100;
}

// === CHANNEL CONVERSION ===

bool AudioOptimizer::ConvertChannels(const std::string& inputPath, const std::string& outputPath,
                                     int targetChannels) {
    std::cout << "Converting to " << targetChannels << " channels: " << inputPath << "\n";
    return targetChannels == 1 ? StereoToMono(inputPath, outputPath) : 
                                 MonoToStereo(inputPath, outputPath);
}

bool AudioOptimizer::MonoToStereo(const std::string& inputPath, const std::string& outputPath) {
    std::cout << "Converting mono to stereo: " << inputPath << "\n";
    return true;
}

bool AudioOptimizer::StereoToMono(const std::string& inputPath, const std::string& outputPath) {
    std::cout << "Converting stereo to mono: " << inputPath << "\n";
    return true;
}

// === AUDIO STREAMING ===

void AudioOptimizer::EnableStreaming(const std::string& audioPath, bool enable) {
    auto& meta = metadataCache_[audioPath];
    meta.isStreaming = enable;
    
    if (enable) {
        streamingAudio_[audioPath] = true;
        std::cout << "Enabled streaming for: " << audioPath << "\n";
    } else {
        streamingAudio_.erase(audioPath);
    }
}

bool AudioOptimizer::IsStreaming(const std::string& audioPath) const {
    return streamingAudio_.find(audioPath) != streamingAudio_.end();
}

void AudioOptimizer::SetStreamConfig(const std::string& audioPath, const AudioStreamConfig& config) {
    streamConfigs_[audioPath] = config;
}

AudioStreamConfig AudioOptimizer::GetStreamConfig(const std::string& audioPath) const {
    auto it = streamConfigs_.find(audioPath);
    if (it != streamConfigs_.end()) {
        return it->second;
    }
    return AudioStreamConfig();
}

void AudioOptimizer::UpdateStreamingPriorities(const std::vector<std::string>& activeSounds) {
    std::cout << "Updating streaming priorities for " << activeSounds.size() << " sounds\n";
    
    for (const auto& sound : activeSounds) {
        auto& config = streamConfigs_[sound];
        config.priority = 100;  // Active sounds get high priority
    }
}

size_t AudioOptimizer::CalculateStreamingMemory(const AudioStreamConfig& config, int sampleRate, int channels) const {
    size_t bytesPerSample = 2;  // 16-bit samples
    return config.bufferSize * config.numBuffers * channels * bytesPerSample;
}

// === 3D AUDIO ===

void AudioOptimizer::Set3DConfig(const Audio3DConfig& config) {
    audio3DConfig_ = config;
    std::cout << "Updated 3D audio configuration\n";
    std::cout << "  Reference distance: " << config.referenceDistance << "\n";
    std::cout << "  Max distance: " << config.maxDistance << "\n";
    std::cout << "  Rolloff: " << config.rolloffFactor << "\n";
}

float AudioOptimizer::CalculateAttenuation(float distance) const {
    if (distance <= audio3DConfig_.referenceDistance) {
        return 1.0f;
    }
    
    if (distance >= audio3DConfig_.maxDistance) {
        return 0.0f;
    }
    
    if (audio3DConfig_.attenuationModel == "inverse") {
        return audio3DConfig_.referenceDistance / 
               (audio3DConfig_.referenceDistance + audio3DConfig_.rolloffFactor * 
                (distance - audio3DConfig_.referenceDistance));
    } else if (audio3DConfig_.attenuationModel == "linear") {
        return 1.0f - audio3DConfig_.rolloffFactor * 
               (distance - audio3DConfig_.referenceDistance) / 
               (audio3DConfig_.maxDistance - audio3DConfig_.referenceDistance);
    } else if (audio3DConfig_.attenuationModel == "exponential") {
        return std::pow(distance / audio3DConfig_.referenceDistance, 
                       -audio3DConfig_.rolloffFactor);
    }
    
    return 1.0f;  // No attenuation
}

// === AUDIO EFFECTS ===

bool AudioOptimizer::NormalizeAudio(const std::string& inputPath, const std::string& outputPath,
                                   float targetLevel) {
    std::cout << "Normalizing audio: " << inputPath << " to " << targetLevel << "dB\n";
    return true;
}

bool AudioOptimizer::ApplyFade(const std::string& inputPath, const std::string& outputPath,
                              float fadeInTime, float fadeOutTime) {
    std::cout << "Applying fade (in: " << fadeInTime << "s, out: " << fadeOutTime << "s): " 
              << inputPath << "\n";
    return true;
}

bool AudioOptimizer::ApplyCompression(const std::string& inputPath, const std::string& outputPath,
                                     float threshold, float ratio) {
    std::cout << "Applying dynamic range compression: " << inputPath << "\n";
    std::cout << "  Threshold: " << threshold << "dB, Ratio: " << ratio << ":1\n";
    return true;
}

bool AudioOptimizer::ApplyEQ(const std::string& inputPath, const std::string& outputPath,
                            const std::vector<float>& bandGains) {
    std::cout << "Applying EQ with " << bandGains.size() << " bands: " << inputPath << "\n";
    return true;
}

// === QUALITY PRESETS ===

void AudioOptimizer::SetQualityPreset(const std::string& preset) {
    if (preset == "low") {
        currentQuality_ = AudioQuality::Low;
    } else if (preset == "medium") {
        currentQuality_ = AudioQuality::Medium;
    } else if (preset == "high") {
        currentQuality_ = AudioQuality::High;
    } else if (preset == "lossless") {
        currentQuality_ = AudioQuality::Lossless;
    }
    
    std::cout << "Audio quality preset set to: " << preset << "\n";
}

void AudioOptimizer::ApplyQualityToAll(AudioQuality quality) {
    currentQuality_ = quality;
    std::cout << "Applied quality " << static_cast<int>(quality) << " to all audio\n";
}

// === BATCH OPERATIONS ===

int AudioOptimizer::BatchConvert(const std::vector<std::string>& audioFiles, AudioFormat targetFormat) {
    std::cout << "Batch converting " << audioFiles.size() << " audio files\n";
    
    int count = 0;
    for (const auto& path : audioFiles) {
        std::string outPath = path + ".converted";
        if (CompressAudio(path, outPath, targetFormat)) {
            count++;
        }
    }
    
    return count;
}

void AudioOptimizer::NormalizeDirectory(const std::string& directory, float targetLevel, bool recursive) {
    std::cout << "Normalizing all audio in: " << directory 
              << " to " << targetLevel << "dB" 
              << (recursive ? " (recursive)" : "") << "\n";
}

void AudioOptimizer::OptimizeDirectory(const std::string& directory, bool recursive) {
    std::cout << "Optimizing all audio files in: " << directory 
              << (recursive ? " (recursive)" : "") << "\n";
    
    // Real implementation would scan directory and optimize
}

void AudioOptimizer::CompressDirectory(const std::string& directory, AudioFormat format,
                                      AudioQuality quality, bool recursive) {
    std::cout << "Compressing all audio files in: " << directory 
              << " to format " << static_cast<int>(format) 
              << (recursive ? " (recursive)" : "") << "\n";
    
    // Real implementation would scan directory and compress
}

// === MEMORY MANAGEMENT ===

size_t AudioOptimizer::GetTotalAudioMemory() const {
    size_t total = 0;
    for (const auto& [path, meta] : metadataCache_) {
        total += meta.isStreaming ? 
               CalculateStreamingMemory(GetStreamConfig(path), 
                                       meta.sampleRate, 
                                       meta.channels) :
               meta.memorySize;
    }
    return total;
}

void AudioOptimizer::UnloadLeastRecentlyUsed(size_t targetBytes) {
    std::cout << "Unloading audio to free: " << (targetBytes / 1024) << "KB\n";
    
    size_t freedBytes = 0;
    std::vector<std::string> toUnload;
    
    for (const auto& [path, meta] : metadataCache_) {
        if (freedBytes < targetBytes) {
            toUnload.push_back(path);
            freedBytes += meta.memorySize;
        }
    }
    
    for (const auto& path : toUnload) {
        UnloadAudio(path);
    }
}

// === AUDIO POOLING ===

void AudioOptimizer::PreloadAudio(const std::string& audioPath) {
    std::cout << "Preloading audio: " << audioPath << "\n";
}

void AudioOptimizer::UnloadAudio(const std::string& audioPath) {
    std::cout << "Unloading audio: " << audioPath << "\n";
    metadataCache_.erase(audioPath);
}

void AudioOptimizer::PreloadCategory(const std::string& category) {
    std::cout << "Preloading audio category: " << category << "\n";
}

void AudioOptimizer::UnloadCategory(const std::string& category) {
    std::cout << "Unloading audio category: " << category << "\n";
}

void AudioOptimizer::SetCategoryVolume(const std::string& category, float volume) {
    categoryVolumes_[category] = volume;
    std::cout << "Set volume for category " << category << " to " << volume << "\n";
}

// === AUDIO METADATA ===

AudioMetadata AudioOptimizer::GetMetadata(const std::string& audioPath) {
    auto it = metadataCache_.find(audioPath);
    if (it != metadataCache_.end()) {
        return it->second;
    }
    return AudioMetadata();
}

void AudioOptimizer::CacheMetadata(const std::string& audioPath, const AudioMetadata& metadata) {
    metadataCache_[audioPath] = metadata;
}

size_t AudioOptimizer::AnalyzeMemoryUsage(const std::string& audioPath) {
    auto it = metadataCache_.find(audioPath);
    if (it != metadataCache_.end()) {
        return it->second.memorySize;
    }
    return 0;
}

std::vector<std::string> AudioOptimizer::FindUnoptimizedAudio(size_t minSize) {
    std::vector<std::string> unoptimized;
    for (const auto& [path, meta] : metadataCache_) {
        if (meta.memorySize >= minSize && meta.format == AudioFormat::WAV_PCM) {
            unoptimized.push_back(path);
        }
    }
    return unoptimized;
}

// === MEMORY MANAGEMENT ===

size_t AudioOptimizer::GetStreamingAudioMemory() const {
    size_t total = 0;
    for (const auto& [path, meta] : metadataCache_) {
        if (meta.isStreaming) {
            total += CalculateStreamingMemory(GetStreamConfig(path), meta.sampleRate, meta.channels);
        }
    }
    return total;
}

void AudioOptimizer::SetMemoryBudget(size_t maxBytes) {
    memoryBudget_ = maxBytes;
    std::cout << "Audio memory budget set to: " << (maxBytes / 1024 / 1024) << "MB\n";
}

bool AudioOptimizer::IsWithinMemoryBudget() const {
    return GetTotalAudioMemory() <= memoryBudget_;
}

// === DIAGNOSTICS ===

void AudioOptimizer::DumpAudioReport(const std::string& outputPath) {
    std::cout << "Audio Optimization Report:\n";
    std::cout << "  Total audio files: " << metadataCache_.size() << "\n";
    std::cout << "  Total memory: " << (GetTotalAudioMemory() / 1024 / 1024) << "MB\n";
    std::cout << "  Streaming memory: " << (GetStreamingAudioMemory() / 1024 / 1024) << "MB\n";
}

int AudioOptimizer::GetLoadedAudioCount() const {
    return metadataCache_.size();
}

int AudioOptimizer::GetStreamingAudioCount() const {
    return streamingAudio_.size();
}

void AudioOptimizer::ClearCache() {
    metadataCache_.clear();
    std::cout << "Audio metadata cache cleared\n";
}

std::string AudioOptimizer::GetOptimizationReport() const {
    std::string report = "Audio Optimization Summary:\n";
    report += "  Loaded: " + std::to_string(GetLoadedAudioCount()) + " files\n";
    report += "  Streaming: " + std::to_string(GetStreamingAudioCount()) + " files\n";
    report += "  Total Memory: " + std::to_string(GetTotalAudioMemory() / 1024 / 1024) + "MB\n";
    report += "  Budget: " + std::to_string(memoryBudget_ / 1024 / 1024) + "MB\n";
    return report;
}

// === 3D AUDIO ===

void AudioOptimizer::UpdateListenerPosition(float x, float y, float z, float vx, float vy, float vz) {
    listenerState_.x = x;
    listenerState_.y = y;
    listenerState_.z = z;
    listenerState_.vx = vx;
    listenerState_.vy = vy;
    listenerState_.vz = vz;
}

void AudioOptimizer::Update3DSource(const std::string& audioPath, float x, float y, float z,
                                   float vx, float vy, float vz) {
    std::cout << "Updated 3D source " << audioPath << " position: (" << x << ", " << y << ", " << z << ")\n";
}

} // namespace Nova
