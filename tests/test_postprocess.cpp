/**
 * Simple test for PostProcessPipeline initialization and API
 */
#include "../src/PostProcessPipeline.h"
#include <iostream>
#include <cassert>

void TestAPI() {
    std::cout << "Testing PostProcessPipeline API..." << std::endl;
    
    PostProcessPipeline pipeline;
    
    // Initially not initialized
    assert(!pipeline.IsInitialized());
    
    // Test setters before initialization (should not crash)
    pipeline.SetBloomEnabled(true);
    pipeline.SetLetterboxEnabled(false);
    pipeline.SetBloomIntensity(0.5f);
    pipeline.SetBloomThreshold(0.8f);
    pipeline.SetLetterboxHeight(0.15f);
    
    // Test getters
    assert(pipeline.IsBloomEnabled() == true);
    assert(pipeline.IsLetterboxEnabled() == false);
    
    std::cout << "  API tests passed" << std::endl;
}

void TestLifecycle() {
    std::cout << "Testing PostProcessPipeline lifecycle..." << std::endl;
    
    PostProcessPipeline pipeline;
    
    // Note: Init() will likely fail in a test environment without GL context
    // This is expected - we're just testing that it doesn't crash
    
    // Try to initialize (will fail without GL context, which is fine)
    bool result = pipeline.Init(800, 600);
    std::cout << "  Init result: " << (result ? "SUCCESS" : "FAILED (expected without GL context)") << std::endl;
    
    // Resize should not crash even if not initialized
    pipeline.Resize(1024, 768);
    
    // BeginScene/EndScene should not crash
    pipeline.BeginScene();
    pipeline.EndScene();
    
    // Shutdown should be safe to call multiple times
    pipeline.Shutdown();
    pipeline.Shutdown();
    
    std::cout << "  Lifecycle tests passed" << std::endl;
}

int main() {
    std::cout << "Running PostProcessPipeline Tests" << std::endl;
    std::cout << "=================================" << std::endl;
    
    TestAPI();
    TestLifecycle();
    
    std::cout << "=================================" << std::endl;
    std::cout << "All tests passed!" << std::endl;
    
    return 0;
}
