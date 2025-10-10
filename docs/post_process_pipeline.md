# Post-Process Pipeline

The Star Engine now includes a modular post-processing pipeline that enables visual effects like bloom and letterbox overlays without modifying the main rendering code.

## Features

### 1. **Letterbox Overlay**
A cinematic letterbox effect that adds black bars to the top and bottom of the screen, creating a widescreen movie-like appearance perfect for HUD overlays.

- **Default**: Enabled
- **Bar Height**: 10% of screen height (configurable)
- **Use Case**: Provides a dedicated area for HUD elements without cluttering the main viewport

### 2. **Bloom Effect**
A simple but effective bloom/glow effect that makes bright areas "bleed" light, adding a soft, ethereal quality to the visuals.

- **Default**: Disabled
- **Intensity**: 0.8 (configurable 0.0-1.0)
- **Threshold**: 0.7 (brightness level to trigger bloom)
- **Implementation**: Bright pass extraction + multi-pass Gaussian blur + additive compositing

## Architecture

The pipeline uses OpenGL Framebuffer Objects (FBO) to implement render-to-texture:

```
Main Scene → Scene FBO → Bright Pass → Blur Passes → Screen Composite
                                                     ↓
                                              + Letterbox Overlay
```

### Render Flow

1. **BeginScene()**: Binds the scene FBO, redirecting all rendering to an offscreen texture
2. **Scene Rendering**: All normal rendering happens (3D models, particles, HUD, etc.)
3. **EndScene()**: Applies post-process effects and composites to the screen
   - Extract bright areas (if bloom enabled)
   - Apply blur passes (if bloom enabled)
   - Composite scene + bloom to screen
   - Draw letterbox bars (if enabled)

## API

### C++ Interface

```cpp
// Access the pipeline through Viewport3D
Viewport3D* viewport = GetViewport();
PostProcessPipeline* pipeline = viewport->GetPostProcessPipeline();

// Toggle effects
viewport->SetBloomEnabled(true);
viewport->SetLetterboxEnabled(false);

// Configure bloom
pipeline->SetBloomIntensity(0.5f);     // 0.0 = subtle, 1.0 = intense
pipeline->SetBloomThreshold(0.8f);     // Higher = only very bright areas bloom

// Configure letterbox
pipeline->SetLetterboxHeight(0.15f);   // 15% of screen height
```

### Runtime Controls

Add keyboard controls to toggle effects at runtime:

```cpp
// In your input handling code:
if (keyPressed('B')) {
    bool bloom = viewport->IsBloomEnabled();
    viewport->SetBloomEnabled(!bloom);
    std::cout << "Bloom: " << (!bloom ? "ON" : "OFF") << std::endl;
}

if (keyPressed('L')) {
    bool letterbox = viewport->IsLetterboxEnabled();
    viewport->SetLetterboxEnabled(!letterbox);
    std::cout << "Letterbox: " << (!letterbox ? "ON" : "OFF") << std::endl;
}
```

## Technical Details

### Framebuffer Layout

- **Scene FBO**: Full resolution (width × height)
  - Color texture: RGBA8
  - Depth renderbuffer: 24-bit
  
- **Bright Pass FBO**: Half resolution (width/2 × height/2)
  - Color texture: RGBA8
  - Extracts bright areas for bloom
  
- **Blur FBOs**: Quarter resolution (width/4 × height/4)
  - 2 textures for ping-pong blur
  - Multi-pass box blur (simulates Gaussian)

### Performance Considerations

- **Downsampling**: Bloom uses quarter-resolution buffers to reduce fillrate cost
- **Pass Count**: Default 2 blur passes (configurable)
- **Fallback**: If FBO extensions unavailable, pipeline gracefully disables itself
- **Memory**: ~4-5 MB for 1920×1080 (all buffers combined)

### Compatibility

- **Requires**: OpenGL FBO extensions (GL_EXT_framebuffer_object)
- **Supported**: OpenGL 2.0+ (extensions) or OpenGL 3.0+ (core)
- **Fallback**: Gracefully disables if FBO not supported
- **Tested On**: Windows (GLFW), Linux (planned)

## Future Enhancements

Potential additions to the post-process pipeline:

- [ ] **Depth of Field**: Blur based on distance from focal plane
- [ ] **Motion Blur**: Velocity-based blur for fast-moving objects
- [ ] **Color Grading**: LUT-based color correction
- [ ] **Vignette**: Darken screen edges for focus
- [ ] **Chromatic Aberration**: Simulate lens imperfections
- [ ] **Screen Space Reflections**: Glossy surface reflections
- [ ] **Antialiasing**: FXAA or SMAA post-process AA
- [ ] **Tone Mapping**: HDR to LDR conversion with exposure

## Troubleshooting

### "FBO extensions not supported"
Your graphics driver doesn't support framebuffer objects. This is extremely rare on modern systems (post-2006). Update your graphics drivers.

### Poor performance
- Reduce blur passes from 2 to 1
- Disable bloom when not needed
- Check GPU load (should be <5ms for post-processing at 1080p)

### Visual artifacts
- Check for FBO binding errors in console
- Ensure depth testing is properly restored after effects
- Verify texture clamping is set correctly

## References

- [OpenGL Framebuffer Objects](https://www.khronos.org/opengl/wiki/Framebuffer_Object)
- [Real-Time Rendering, 4th Ed.](http://www.realtimerendering.com/) - Chapter on Post-Processing
- [GPU Gems: Efficient Gaussian Blur](https://developer.nvidia.com/gpugems/gpugems3/part-vi-gpu-computing/chapter-40-incremental-computation-gaussian)
