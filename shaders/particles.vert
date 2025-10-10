#version 460 core

// Vertex attributes
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;

// Outputs to fragment shader
out vec4 vColor;

// Uniforms
uniform mat4 uViewProjection;

void main() {
    // Transform position to clip space
    gl_Position = uViewProjection * vec4(aPosition, 1.0);
    
    // Set point size (for point sprites)
    gl_PointSize = aSize;
    
    // Pass color to fragment shader
    vColor = aColor;
}
