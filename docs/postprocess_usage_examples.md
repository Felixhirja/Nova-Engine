# Post-Process Pipeline Usage Examples

## Basic Usage

### Enabling/Disabling Effects

```cpp
// Get viewport reference
Viewport3D* viewport = mainLoop.GetViewport();

// Toggle bloom
viewport->SetBloomEnabled(true);   // Enable bloom
viewport->SetBloomEnabled(false);  // Disable bloom

// Toggle letterbox
viewport->SetLetterboxEnabled(true);   // Enable letterbox
viewport->SetLetterboxEnabled(false);  // Disable letterbox

// Check current state
bool bloomActive = viewport->IsBloomEnabled();
bool letterboxActive = viewport->IsLetterboxEnabled();
```

### Advanced Configuration

```cpp
// Access the pipeline directly for fine-tuning
PostProcessPipeline* pipeline = viewport->GetPostProcessPipeline();

// Configure bloom parameters
pipeline->SetBloomIntensity(0.5f);    // Subtle glow (0.0 - 1.0)
pipeline->SetBloomThreshold(0.8f);    // Only very bright areas bloom

// Configure letterbox
pipeline->SetLetterboxHeight(0.15f);  // 15% bars (larger = more cinematic)
```

## Runtime Controls (Already Implemented)

The engine includes keyboard shortcuts:
- **B key**: Toggle bloom effect
- **L key**: Toggle letterbox overlay

## Integration Example

If you're adding a settings menu or configuration system:

```cpp
// Example settings structure
struct GraphicsSettings {
    bool bloomEnabled = false;
    float bloomIntensity = 0.8f;
    bool letterboxEnabled = true;
    float letterboxHeight = 0.1f;
};

// Apply settings to engine
void ApplyGraphicsSettings(const GraphicsSettings& settings, Viewport3D* viewport) {
    // Enable/disable effects
    viewport->SetBloomEnabled(settings.bloomEnabled);
    viewport->SetLetterboxEnabled(settings.letterboxEnabled);
    
    // Configure parameters
    PostProcessPipeline* pipeline = viewport->GetPostProcessPipeline();
    pipeline->SetBloomIntensity(settings.bloomIntensity);
    pipeline->SetLetterboxHeight(settings.letterboxHeight);
}
```

## Performance Tuning

For lower-end hardware, you can disable effects conditionally:

```cpp
// Check if post-processing is available
if (viewport->GetPostProcessPipeline()->IsInitialized()) {
    // Enable effects on capable hardware
    viewport->SetBloomEnabled(true);
} else {
    std::cout << "Post-processing not available on this system" << std::endl;
}

// Or disable based on performance metrics
if (currentFPS < 30.0) {
    viewport->SetBloomEnabled(false);  // Disable bloom to improve FPS
}
```

## Extending the Pipeline

To add new effects, modify `PostProcessPipeline.cpp`:

```cpp
// Example: Add a new effect method
void PostProcessPipeline::ApplyVignette() {
    // Darken screen edges
    // Implementation would go here
}

// Call from EndScene()
void PostProcessPipeline::EndScene() {
    // ... existing code ...
    
    if (vignetteEnabled_) {
        ApplyVignette();
    }
    
    CompositeToScreen();
}
```

## Troubleshooting

### Pipeline Not Initializing
If you see "Post-process pipeline initialization failed":

```cpp
// Check for FBO support
if (!viewport->GetPostProcessPipeline()->IsInitialized()) {
    // Fallback: disable post-processing UI options
    // Continue with normal rendering
}
```

### Performance Issues
Monitor frame time and adjust accordingly:

```cpp
// Simple performance monitoring
if (frameTime > 33.0) {  // Less than 30 FPS
    viewport->SetBloomEnabled(false);
    std::cout << "Bloom disabled to improve performance" << std::endl;
}
```
