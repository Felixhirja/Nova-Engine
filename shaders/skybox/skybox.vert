#version 110

// Skybox vertex shader
// Renders a cube at infinite distance

attribute vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

varying vec3 TexCoords;

void main() {
    TexCoords = aPos;
    
    // Remove translation from view matrix (keep rotation only)
    mat4 rotView = mat4(mat3(view));
    vec4 pos = projection * rotView * vec4(aPos, 1.0);
    
    // Set z = w to ensure maximum depth after perspective divide
    // This makes the skybox render behind everything else
    gl_Position = pos.xyww;
}
