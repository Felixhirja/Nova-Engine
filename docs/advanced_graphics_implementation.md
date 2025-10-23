# Advanced Graphics System Implementation

## Overview
This document outlines the implementation of advanced graphics features for Nova-Engine, building on the existing post-process pipeline.

## Implementation Phases

### ✅ Phase 0: Foundation (COMPLETE)
- Post-process pipeline with bloom and letterbox
- Fixed-function OpenGL rendering
- Basic mesh rendering system

### Phase 1: Shader System (NEW)
Modern shader management infrastructure for programmable pipeline.

**Components:**
- `ShaderProgram` - Shader compilation and linking
- `ShaderManager` - Resource management and caching
- `ShaderLibrary` - Built-in shader collection

**Features:**
- GLSL vertex and fragment shader support
- Uniform variable management
- Automatic shader hot-reloading (dev mode)
- Error reporting with line numbers
- Shader preprocessing (#include support)

**Time Estimate:** 8-12 hours

### Phase 2: Skybox System
Immersive space environment rendering with cubemaps.

**Components:**
- `Skybox` - Cubemap rendering
- `CubemapLoader` - 6-sided texture loading
- Built-in procedural space skybox generator

**Features:**
- 360° environment mapping
- Procedural starfield generation
- Nebula/galaxy textures
- Dynamic time-of-day (for planetary scenes)
- HDR skybox support

**Time Estimate:** 6-10 hours

### Phase 3: Shadow Mapping
Real-time dynamic shadows for enhanced depth perception.

**Components:**
- `ShadowMap` - Shadow texture generation
- `LightManager` - Light source management
- Shadow cascades for large scenes

**Features:**
- Directional light shadows (sun/star)
- Point light shadows (omnidirectional)
- Soft shadows with PCF (Percentage Closer Filtering)
- Shadow bias and acne prevention
- Performance-optimized shadow resolution

**Time Estimate:** 12-16 hours

### Phase 4: Motion Blur
Cinematic motion blur for fast-moving objects and camera movement.

**Components:**
- Velocity buffer rendering
- Temporal motion vectors
- Screen-space blur kernel

**Features:**
- Camera motion blur
- Per-object motion blur
- Configurable blur strength
- Performance optimizations

**Time Estimate:** 4-6 hours

## Architecture

### Rendering Pipeline Evolution

**Current (Fixed-Function):**
```
Scene → PostProcess (Bloom) → Screen
```

**New (Programmable Pipeline):**
```
Scene Geometry → Shadow Pass → Main Pass (Shaders) → PostProcess → Screen
                      ↓              ↓
                  ShadowMap      Skybox/Lighting
```

### Shader System Architecture

```cpp
// Shader compilation and management
ShaderManager shaders;

// Load shaders
ShaderProgram* basicShader = shaders.Load("basic", "shaders/basic.vert", "shaders/basic.frag");
ShaderProgram* pbrShader = shaders.Load("pbr", "shaders/pbr.vert", "shaders/pbr.frag");

// Use in rendering
basicShader->Use();
basicShader->SetUniform("modelMatrix", modelMatrix);
basicShader->SetUniform("lightPos", lightPosition);
```

### Resource Organization

```
Nova-Engine/
├── shaders/                    # NEW: Shader source files
│   ├── core/
│   │   ├── basic.vert
│   │   ├── basic.frag
│   │   ├── pbr.vert
│   │   ├── pbr.frag
│   │   └── include/
│   │       ├── common.glsl
│   │       └── lighting.glsl
│   ├── skybox/
│   │   ├── skybox.vert
│   │   ├── skybox.frag
│   │   └── starfield.frag
│   ├── shadows/
│   │   ├── shadow.vert
│   │   ├── shadow.frag
│   │   └── pcf_shadows.glsl
│   └── postprocess/
│       ├── motion_blur.frag
│       └── velocity_buffer.frag
├── src/
│   ├── graphics/               # NEW: Graphics subsystem
│   │   ├── ShaderProgram.h/cpp
│   │   ├── ShaderManager.h/cpp
│   │   ├── Skybox.h/cpp
│   │   ├── ShadowMap.h/cpp
│   │   └── LightManager.h/cpp
│   └── PostProcessPipeline.cpp # MODIFY: Add motion blur
├── assets/
│   └── skyboxes/               # NEW: Skybox textures
│       ├── space_nebula/
│       └── solar_system/
└── docs/
    ├── shader_system.md
    ├── skybox_system.md
    └── shadow_mapping.md
```

## Technical Specifications

### Shader System Requirements

**OpenGL Version:**
- Minimum: OpenGL 2.0 + GLSL 110
- Recommended: OpenGL 3.3 + GLSL 330
- Maximum: OpenGL 4.6 + GLSL 460

**Shader Compilation:**
- Compile vertex and fragment shaders
- Link shader program
- Validate uniforms and attributes
- Cache compiled shaders to avoid recompilation

**Error Handling:**
```cpp
if (!shader->Compile()) {
    std::cerr << "Shader compilation failed:\n" 
              << shader->GetErrorLog() << std::endl;
    // Fallback to fixed-function or default shader
}
```

### Skybox Technical Details

**Cubemap Format:**
- 6 textures: +X, -X, +Y, -Y, +Z, -Z
- Resolution: 512×512 to 2048×2048 per face
- Format: RGB8 or RGB16F (HDR)
- Mipmaps: Optional but recommended

**Rendering:**
- Render last (after opaque geometry)
- Disable depth writes, enable depth test
- Use cube mesh centered at camera
- Transform texture coordinates by inverse view matrix

**Procedural Starfield:**
- Generate random star positions
- Size variation (1-3 pixels)
- Color variation (white to blue)
- Twinkle animation (optional)
- 5000-10000 stars for dense field

### Shadow Mapping Technical Details

**Shadow Map Resolution:**
- Low: 512×512 (mobile/low-end)
- Medium: 1024×1024 (default)
- High: 2048×2048 (high-end)
- Ultra: 4096×4096 (enthusiast)

**Shadow Techniques:**
1. **Basic Shadow Mapping**: Simple depth comparison
2. **PCF (Percentage Closer Filtering)**: 3×3 or 5×5 kernel blur
3. **Cascaded Shadow Maps**: Multiple shadow maps for large scenes
4. **Variance Shadow Maps**: Pre-filtered shadows (advanced)

**Performance Optimization:**
- Render only shadow-casting objects
- Use separate low-poly shadow meshes
- Cull objects outside light frustum
- Update only when lights or objects move

### Motion Blur Technical Details

**Implementation:**
- Screen-space velocity buffer
- Per-pixel motion vectors
- Variable blur kernel (3-15 samples)
- Adaptive quality based on motion magnitude

**Shader Approach:**
```glsl
// Compute velocity
vec2 velocity = currentPos - previousPos;

// Sample along motion vector
vec4 color = texture(sceneTexture, uv);
for (int i = 1; i < samples; i++) {
    vec2 offset = velocity * (float(i) / float(samples));
    color += texture(sceneTexture, uv + offset);
}
color /= float(samples);
```

## Performance Targets

| Feature | Target Frame Time | GPU Load |
|---------|------------------|----------|
| Shader System | <0.1ms | Minimal |
| Skybox | 0.2-0.5ms | <5% |
| Shadow Mapping | 1-3ms | 10-20% |
| Motion Blur | 0.5-1.5ms | 5-10% |
| **Total** | **~5ms** | **20-35%** |

At 1920×1080, this keeps the engine comfortably above 60 FPS (16.67ms budget).

## Shader Library

### Basic Shaders

**basic.vert** - Simple vertex transformation
```glsl
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec3 FragPos;
out vec3 Normal;

void main() {
    FragPos = vec3(modelMatrix * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(modelMatrix))) * aNormal;
    gl_Position = projectionMatrix * viewMatrix * vec4(FragPos, 1.0);
}
```

**basic.frag** - Blinn-Phong lighting
```glsl
#version 330 core
in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform vec3 lightColor;

out vec4 FragColor;

void main() {
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
    vec3 specular = spec * lightColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
```

### PBR Shaders (Physically-Based Rendering)

For realistic materials in space environments.

### Skybox Shader

**skybox.vert**
```glsl
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 TexCoords;

void main() {
    TexCoords = aPos;
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    gl_Position = pos.xyww; // Set z = w for max depth
}
```

**skybox.frag**
```glsl
#version 330 core
in vec3 TexCoords;

uniform samplerCube skybox;

out vec4 FragColor;

void main() {
    FragColor = texture(skybox, TexCoords);
}
```

## Migration Path

### Phase 1: Shader System
1. Implement `ShaderProgram` class
2. Implement `ShaderManager` for resource management
3. Create basic vertex/fragment shaders
4. Convert one mesh to use shaders (player ship)
5. Verify rendering matches fixed-function output
6. Migrate remaining geometry

### Phase 2: Skybox
1. Implement `Skybox` class
2. Create procedural starfield generator
3. Add skybox rendering before scene
4. Create manual cubemap loader
5. Add example space skyboxes

### Phase 3: Shadows
1. Implement depth-only shadow pass
2. Create `ShadowMap` class
3. Add shadow projection to main shader
4. Implement PCF for soft shadows
5. Add shadow controls to settings

### Phase 4: Motion Blur
1. Add velocity buffer to post-process pipeline
2. Implement motion vector calculation
3. Create motion blur shader
4. Add blur kernel sampling
5. Add toggle and intensity controls

## Testing Strategy

### Unit Tests
- Shader compilation success/failure
- Uniform setting and retrieval
- Resource cleanup and leaks
- Cubemap loading

### Visual Tests
- Shader rendering correctness
- Skybox seamless edges
- Shadow alignment and bias
- Motion blur artifact detection

### Performance Tests
- Frame time profiling
- GPU memory usage
- Shader compilation time
- Shadow map update frequency

## Fallback Strategy

All features have graceful degradation:

1. **Shaders fail → Fixed-function rendering**
2. **Skybox fails → Solid background color**
3. **Shadows fail → Unshadowed rendering**
4. **Motion blur fails → No blur**

## References

- [Learn OpenGL - Shaders](https://learnopengl.com/Getting-started/Shaders)
- [Learn OpenGL - Cubemaps](https://learnopengl.com/Advanced-OpenGL/Cubemaps)
- [Learn OpenGL - Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping)
- [GPU Gems - Motion Blur](https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect)
- [Shader Quality - Inigo Quilez](https://iquilezles.org/articles/)

## Status Tracking

| Feature | Status | Priority | Estimated Time |
|---------|--------|----------|----------------|
| Shader System | 🔴 Not Started | HIGH | 8-12 hours |
| Skybox System | 🔴 Not Started | MEDIUM | 6-10 hours |
| Shadow Mapping | 🔴 Not Started | MEDIUM | 12-16 hours |
| Motion Blur | 🔴 Not Started | LOW | 4-6 hours |
| **Total** | | | **30-44 hours** |

Legend: 🔴 Not Started | 🟡 In Progress | 🟢 Complete

## TODO Checklist

- [ ] Kick off **Phase 1: Shader System** implementation (`ShaderProgram`, `ShaderManager`, initial mesh conversion) and update the "Phase 4" entry in `docs/todo/TODO_LIST.txt` when work begins.
- [ ] Prepare asset and documentation notes for **Phase 2: Skybox System** (procedural starfield + cubemap loader) before integrating into `Viewport3D`.
- [ ] Define testing plan and engine hooks for **Phase 3: Shadow Mapping** so that QA tasks can be added to `docs/todo/TODO_LIST.txt` alongside rendering milestones.
- [ ] Prototype **Phase 4: Motion Blur** within the post-process pipeline and record performance metrics for future TODO tracking.

---
*Last Updated: October 10, 2025*
