# Shader System Quick Reference

## Overview
The shader system provides a modern programmable rendering pipeline for Star-Engine, enabling advanced visual effects while maintaining compatibility with older hardware.

## Quick Start

### 1. Basic Shader Usage

```cpp
#include "graphics/ShaderProgram.h"

// Load shader
ShaderProgram shader;
if (!shader.LoadFromFiles("shaders/core/basic.vert", "shaders/core/basic.frag")) {
    std::cerr << "Shader failed: " << shader.GetErrorLog() << std::endl;
    // Fallback to fixed-function rendering
}

// Use in render loop
shader.Use();
shader.SetUniform("lightPos", lightX, lightY, lightZ);
shader.SetUniform("objectColor", 1.0f, 0.5f, 0.0f);
// Draw geometry...
ShaderProgram::Unuse();
```

### 2. Setting Uniforms

```cpp
// Scalar values
shader.SetUniform("time", elapsedTime);
shader.SetUniform("brightness", 1.5f);

// Vectors
shader.SetUniform("position", x, y);           // vec2
shader.SetUniform("color", r, g, b);           // vec3
shader.SetUniform("rotation", x, y, z, w);     // vec4

// Matrices (4x4, column-major)
float modelMatrix[16] = { /* ... */ };
shader.SetUniformMatrix4("modelMatrix", modelMatrix);

// Textures
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, textureID);
shader.SetUniformTexture("diffuseMap", 0);  // Texture unit 0
```

### 3. Error Handling

```cpp
if (!shader.LoadFromFiles("vertex.glsl", "fragment.glsl")) {
    std::cout << "Shader compilation failed:\n" 
              << shader.GetErrorLog() << std::endl;
    // Errors include line numbers and descriptions
}
```

### 4. Hot Reloading (Development)

```cpp
// Reload shaders without restarting engine
if (keyPressed('R')) {
    if (shader.Reload()) {
        std::cout << "Shaders reloaded successfully" << std::endl;
    }
}
```

## Built-in Shaders

### basic.vert / basic.frag
Simple Blinn-Phong lighting for solid objects.

**Uniforms:**
- `modelMatrix`, `viewMatrix`, `projectionMatrix` (mat4)
- `lightPos`, `viewPos` (vec3) - World space positions
- `objectColor`, `lightColor` (vec3) - RGB colors
- `ambientStrength` (float) - Default: 0.1
- `specularStrength` (float) - Default: 0.5

**Attributes:**
- `aPos` (vec3) - Vertex position
- `aNormal` (vec3) - Vertex normal

### skybox.vert / skybox.frag
Cubemap environment rendering.

**Uniforms:**
- `projection`, `view` (mat4)
- `skybox` (samplerCube) - Cubemap texture

### starfield.frag
Procedural star generation (no textures).

**Uniforms:**
- `time` (float) - For star twinkling
- `starDensity` (float) - 0.001-0.01
- `starBrightness` (float) - 0.5-2.0

## Common Patterns

### Per-Object Rendering

```cpp
shader.Use();
shader.SetUniformMatrix4("viewMatrix", viewMatrix);
shader.SetUniformMatrix4("projectionMatrix", projMatrix);

for (auto& object : objects) {
    shader.SetUniformMatrix4("modelMatrix", object.transform);
    shader.SetUniform("objectColor", object.r, object.g, object.b);
    object.mesh.Draw();
}

ShaderProgram::Unuse();
```

### Multi-Pass Rendering

```cpp
// Pass 1: Shadow map
shadowShader.Use();
// Render depth only...
ShaderProgram::Unuse();

// Pass 2: Main render
mainShader.Use();
shadowShader.SetUniformTexture("shadowMap", 0);
// Render scene with shadows...
ShaderProgram::Unuse();
```

### Shader Variants

```cpp
// Load multiple shader variants
ShaderProgram basicShader, litShader, pbrShader;
basicShader.LoadFromFiles("basic.vert", "basic.frag");
litShader.LoadFromFiles("lit.vert", "lit.frag");
pbrShader.LoadFromFiles("pbr.vert", "pbr.frag");

// Choose based on material quality
if (material.usePBR) {
    pbrShader.Use();
} else if (material.useLighting) {
    litShader.Use();
} else {
    basicShader.Use();
}
```

## Performance Tips

1. **Minimize State Changes**: Group objects by shader to reduce `Use()` calls
2. **Cache Uniforms**: Uniform locations are automatically cached
3. **Batch Geometry**: Draw multiple objects with same shader before switching
4. **Avoid Redundant Sets**: Don't set uniforms that haven't changed

## Troubleshooting

### "Shader extensions not supported"
Your GPU doesn't support OpenGL 2.0+ shaders. Engine will fallback to fixed-function.

### "Uniform not found" warnings
The uniform was optimized out by the compiler (not used in shader). This is normal.

### Black/incorrect rendering
- Check uniform names match shader code
- Verify matrices are in correct format (column-major)
- Ensure shader compiles without errors

### Performance issues
- Profile with GPU tools (NVIDIA Nsight, RenderDoc)
- Check fragment shader complexity
- Reduce texture samples per pixel

## GLSL Compatibility

**Target:** GLSL 110 (OpenGL 2.0)
- Maximum compatibility with older hardware
- Use `attribute` instead of `in` (vertex)
- Use `varying` instead of `in/out` (between stages)
- Use `gl_FragColor` instead of `out vec4`
- Use `texture2D()` instead of `texture()`
- Use `textureCube()` for cubemaps

**Modern GLSL 330+** support can be added later with version detection.

## Examples

### Pulsing Glow Effect

```glsl
// Fragment shader
uniform float time;
uniform vec3 glowColor;

void main() {
    float pulse = 0.5 + 0.5 * sin(time * 2.0);
    vec3 color = glowColor * pulse;
    gl_FragColor = vec4(color, 1.0);
}
```

### Distance Fog

```glsl
// Fragment shader
uniform vec3 fogColor;
uniform float fogNear;
uniform float fogFar;

void main() {
    // ... lighting calculations ...
    
    float depth = gl_FragCoord.z / gl_FragCoord.w;
    float fogFactor = clamp((fogFar - depth) / (fogFar - fogNear), 0.0, 1.0);
    vec3 finalColor = mix(fogColor, lightedColor, fogFactor);
    
    gl_FragColor = vec4(finalColor, 1.0);
}
```

## References

- [GLSL 1.10 Specification](https://www.khronos.org/files/opengl-quick-reference-card.pdf)
- [Learn OpenGL - Shaders](https://learnopengl.com/Getting-started/Shaders)
- [Shader Toy](https://www.shadertoy.com/) - Shader examples
- [The Book of Shaders](https://thebookofshaders.com/) - Learning resource

---
*For advanced features (PBR, shadows), see `shadow_mapping.md` and `advanced_graphics_implementation.md`*
