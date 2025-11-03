#include "engine/AssetOptimizer.h"
#include "engine/TextureOptimizer.h"
#include "engine/MeshOptimizer.h"
#include "engine/AudioOptimizer.h"
#include "engine/AssetStreamer.h"
#include <iostream>
#include <cassert>

using namespace Nova;

void TestAssetOptimizer() {
    std::cout << "\n=== Testing AssetOptimizer ===\n";
    
    auto& optimizer = AssetOptimizer::GetInstance();
    
    // Test profiling
    optimizer.EnableProfiling(true);
    optimizer.StartLoadingProfile("test.obj", "mesh");
    optimizer.EndLoadingProfile("test.obj", 1024 * 1024, 512 * 1024);
    
    auto profile = optimizer.GetLoadingProfile("test.obj");
    std::cout << "Load time: " << profile.loadTimeMs << "ms\n";
    assert(profile.memoryBytes == 1024 * 1024);
    
    // Test memory management
    optimizer.SetMemoryBudget(2ULL * 1024 * 1024 * 1024, 1ULL * 1024 * 1024 * 1024);
    optimizer.UpdateMemoryStats();
    std::cout << "Within budget: " << (optimizer.IsWithinMemoryBudget() ? "Yes" : "No") << "\n";
    
    // Test quality settings
    optimizer.SetQualityLevel(QualityLevel::High);
    auto& settings = optimizer.GetQualitySettings();
    std::cout << "Max texture size: " << settings.maxTextureSize << "\n";
    assert(settings.maxTextureSize == 4096);
    
    // Auto-detect
    optimizer.AutoDetectQualitySettings();
    
    // Platform profiles
    optimizer.SetPlatformProfile("desktop");
    std::cout << "Platform: " << optimizer.GetPlatformProfile() << "\n";
    
    // Export report
    optimizer.ExportProfileReport("test_profile.txt");
    optimizer.DumpOptimizationReport();
    
    std::cout << "✓ AssetOptimizer tests passed\n";
}

void TestTextureOptimizer() {
    std::cout << "\n=== Testing TextureOptimizer ===\n";
    
    auto& texOpt = TextureOptimizer::GetInstance();
    
    // Test format selection
    auto format = texOpt.SelectOptimalFormat(4, true, false, false);
    std::cout << "Selected format for RGBA: " << static_cast<int>(format) << "\n";
    
    // Test compression
    texOpt.CompressTexture("test.png", "test_compressed.dds", 
                          TextureFormat::BC7, 85);
    
    // Test size estimation
    size_t size = texOpt.EstimateCompressedSize(2048, 2048, TextureFormat::DXT5);
    std::cout << "Estimated compressed size: " << (size / 1024) << "KB\n";
    
    // Test mipmaps
    texOpt.GenerateMipmaps("test.png", -1);
    int levels = texOpt.CalculateOptimalMipmapLevels(2048, 2048);
    std::cout << "Optimal mipmap levels: " << levels << "\n";
    assert(levels == 12); // log2(2048) + 1
    
    // Test resizing
    texOpt.ResizeTexture("test.png", "test_512.png", 512, 512);
    
    // Test LOD generation
    texOpt.GenerateLODChain("test.png", 4);
    
    // Test atlas creation
    std::vector<std::string> textures = {"tex1.png", "tex2.png", "tex3.png"};
    texOpt.CreateTextureAtlas(textures, "atlas.png", 4096);
    
    // Test streaming
    texOpt.EnableStreaming("test.png", true);
    std::cout << "Streaming enabled for test.png\n";
    
    // Test quality presets
    texOpt.ApplyQualityPreset("high");
    auto config = texOpt.GetQualityConfig();
    std::cout << "Quality config max res: " << config.maxResolution << "\n";
    
    // Batch operations
    std::cout << "Performing batch operations...\n";
    for (const auto& tex : textures) {
        texOpt.CompressTexture(tex, tex + ".compressed", TextureFormat::BC7, 85);
    }
    
    std::cout << "✓ TextureOptimizer tests passed\n";
}

void TestMeshOptimizer() {
    std::cout << "\n=== Testing MeshOptimizer ===\n";
    
    auto& meshOpt = MeshOptimizer::GetInstance();
    
    // Test mesh optimization
    MeshOptimizationConfig config;
    config.optimizeVertexCache = true;
    config.optimizeOverdraw = true;
    config.generateNormals = true;
    
    meshOpt.OptimizeMesh("model.obj", "model_opt.obj", config);
    
    // Test vertex cache optimization
    std::vector<unsigned int> indices = {0, 1, 2, 1, 3, 2};
    meshOpt.OptimizeVertexCache(indices);
    
    float acmr = meshOpt.CalculateACMR(indices, 4, 32);
    std::cout << "ACMR: " << acmr << "\n";
    
    // Test simplification
    meshOpt.SimplifyMesh("model.obj", "model_simple.obj", 0.5f, 0.01f);
    
    int targetTriangles = meshOpt.CalculateTargetTriangleCount(1000, 0.5f);
    std::cout << "Target triangles: " << targetTriangles << "\n";
    assert(targetTriangles == 500);
    
    // Test LOD generation
    LODConfig lodConfig;
    lodConfig.autoGenerate = true;
    
    LODLevel lod0 = {0.0f, 1.0f, 10000};
    LODLevel lod1 = {50.0f, 0.5f, 5000};
    LODLevel lod2 = {100.0f, 0.25f, 2500};
    
    lodConfig.levels = {lod0, lod1, lod2};
    
    meshOpt.GenerateLODChain("model.obj", lodConfig);
    
    int lodLevel = meshOpt.SelectLODLevel("model.obj", 75.0f);
    std::cout << "Selected LOD level at 75m: " << lodLevel << "\n";
    
    // Test mesh analysis
    auto stats = meshOpt.AnalyzeMesh("model.obj");
    std::cout << "Vertices: " << stats.vertexCount << ", Triangles: " << stats.triangleCount << "\n";
    
    // Test geometry processing
    std::vector<float> vertices;
    meshOpt.GenerateNormals(vertices, indices, 8, true);
    meshOpt.GenerateTangents(vertices, indices, 8);
    
    // Test compression
    meshOpt.CompressMesh("model.obj", "model.compressed", 5);
    
    // Test instancing
    meshOpt.MarkForInstancing("model.obj");
    std::cout << "Marked for instancing\n";
    
    // Batch operations
    std::vector<std::string> meshes = {"mesh1.obj", "mesh2.obj", "mesh3.obj"};
    for (const auto& mesh : meshes) {
        meshOpt.OptimizeMeshInPlace(mesh, config);
    }
    
    std::cout << "✓ MeshOptimizer tests passed\n";
}

void TestAudioOptimizer() {
    std::cout << "\n=== Testing AudioOptimizer ===\n";
    
    auto& audioOpt = AudioOptimizer::GetInstance();
    
    // Test format selection
    auto format = audioOpt.SelectOptimalFormat(true, false, true);
    std::cout << "Selected format for music: " << static_cast<int>(format) << "\n";
    
    // Test compression
    audioOpt.CompressAudio("music.wav", "music.ogg", 
                          AudioFormat::OGG_Vorbis, AudioQuality::High);
    
    // Test sample rate conversion
    int sampleRate = audioOpt.SelectOptimalSampleRate(AudioQuality::High);
    std::cout << "Optimal sample rate: " << sampleRate << "Hz\n";
    assert(sampleRate == 44100);
    
    audioOpt.ResampleAudio("sound.wav", "sound_22k.wav", 22050, true);
    
    // Test channel conversion
    audioOpt.StereoToMono("music.wav", "music_mono.wav");
    audioOpt.MonoToStereo("voice.wav", "voice_stereo.wav");
    
    // Test streaming
    audioOpt.EnableStreaming("music.ogg", true);
    assert(audioOpt.IsStreaming("music.ogg"));
    
    AudioStreamConfig streamConfig;
    streamConfig.bufferSize = 4096;
    streamConfig.numBuffers = 4;
    audioOpt.SetStreamConfig("music.ogg", streamConfig);
    
    size_t streamMemory = audioOpt.CalculateStreamingMemory(streamConfig, 44100, 2);
    std::cout << "Streaming memory: " << (streamMemory / 1024) << "KB\n";
    
    // Test 3D audio
    Audio3DConfig audio3D;
    audio3D.maxDistance = 500.0f;
    audio3D.rolloffFactor = 1.0f;
    audioOpt.Set3DConfig(audio3D);
    
    float attenuation = audioOpt.CalculateAttenuation(100.0f);
    std::cout << "Attenuation at 100m: " << attenuation << "\n";
    
    // Test audio effects
    audioOpt.NormalizeAudio("sound.wav", "sound_norm.wav", -3.0f);
    audioOpt.ApplyFade("music.wav", "music_fade.wav", true, 2.0f);
    audioOpt.ApplyCompression("voice.wav", "voice_comp.wav", -20.0f, 4.0f);
    
    // Test quality presets
    audioOpt.SetQualityPreset("high");
    std::cout << "Quality preset set to high\n";
    
    // Batch operations (simulated)
    std::vector<std::string> audioFiles = {"s1.wav", "s2.wav", "s3.wav"};
    for (const auto& audio : audioFiles) {
        audioOpt.CompressAudio(audio, audio + ".ogg", AudioFormat::OGG_Vorbis, AudioQuality::Medium);
    }
    
    // Memory management
    size_t totalMemory = audioOpt.GetTotalAudioMemory();
    std::cout << "Total audio memory: " << (totalMemory / 1024) << "KB\n";
    
    std::cout << "✓ AudioOptimizer tests passed\n";
}

void TestAssetStreamer() {
    std::cout << "\n=== Testing AssetStreamer ===\n";
    
    auto& streamer = AssetStreamer::GetInstance();
    
    // Initialize
    streamer.Initialize(2);
    assert(streamer.IsInitialized());
    
    // Test basic streaming
    bool loaded = false;
    streamer.RequestAsset("model.obj", AssetType::Mesh, 
                         StreamPriority::High,
                         [&loaded](bool success) {
                             loaded = success;
                             std::cout << "Asset loaded: " << (success ? "Yes" : "No") << "\n";
                         });
    
    // Test batch requests
    std::vector<std::string> assets = {"tex1.png", "tex2.png", "tex3.png"};
    streamer.RequestAssets(assets, AssetType::Texture, StreamPriority::Normal);
    
    // Test priority management
    streamer.SetPriority("model.obj", StreamPriority::Critical);
    streamer.BoostPriority("important.obj");
    
    std::vector<std::string> visible = {"model.obj", "tex1.png"};
    streamer.UpdatePriorities(visible);
    
    // Test distance-based streaming
    streamer.UpdateCameraPosition(0.0f, 0.0f, 0.0f);
    streamer.SetStreamingDistance(500.0f);
    
    streamer.RegisterAssetPosition("distant_model.obj", 1000.0f, 0.0f, 0.0f);
    streamer.RegisterAssetPosition("nearby_model.obj", 50.0f, 0.0f, 0.0f);
    
    streamer.UpdateDistanceBasedPriorities();
    
    // Test LOD streaming
    streamer.EnableLODStreaming(true);
    streamer.RequestLODLevel("model.obj", 1);
    
    int lodLevel = streamer.SelectOptimalLOD("model.obj", 75.0f);
    std::cout << "Optimal LOD at 75m: " << lodLevel << "\n";
    
    // Test bandwidth management
    streamer.SetBandwidthLimit(10 * 1024 * 1024); // 10 MB/s
    streamer.EnableBandwidthThrottling(true);
    
    double bandwidth = streamer.GetCurrentBandwidth();
    std::cout << "Current bandwidth: " << bandwidth << " MB/s\n";
    
    // Test prefetching
    streamer.EnablePredictiveLoading(true);
    std::vector<std::string> prefetch = {"level2.obj", "level2_tex.png"};
    streamer.PrefetchAssets(prefetch);
    
    // Test memory management
    streamer.SetMemoryBudget(512 * 1024 * 1024); // 512 MB
    std::cout << "Memory budget set\n";
    std::cout << "Within budget: " << (streamer.IsWithinMemoryBudget() ? "Yes" : "No") << "\n";
    
    streamer.UnloadDistantAssets(1000.0f);
    
    // Test state queries
    auto state = streamer.GetAssetState("model.obj");
    std::cout << "Asset state: " << static_cast<int>(state) << "\n";
    
    // Wait a bit for async operations
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Test status report
    std::string report = streamer.GetStatusReport();
    std::cout << report;
    
    // Cleanup
    streamer.ClearQueue();
    streamer.Shutdown();
    
    std::cout << "✓ AssetStreamer tests passed\n";
}

int main() {
    std::cout << "===================================\n";
    std::cout << "Asset Optimization System Tests\n";
    std::cout << "===================================\n";
    
    try {
        TestAssetOptimizer();
        TestTextureOptimizer();
        TestMeshOptimizer();
        TestAudioOptimizer();
        TestAssetStreamer();
        
        std::cout << "\n===================================\n";
        std::cout << "✓ ALL TESTS PASSED\n";
        std::cout << "===================================\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
