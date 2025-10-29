#version 110

// Basic vertex shader for simple lit geometry
// Compatible with OpenGL 2.0+ / GLSL 110

attribute vec3 aPos;
attribute vec3 aNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec3 FragPos;
varying vec3 Normal;

void main() {
    // Transform position to world space
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    
    // Transform normal to world space without relying on GLSL 1.20 matrix casts
    Normal = normalize((modelMatrix * vec4(aNormal, 0.0)).xyz);
    
    // Final position in clip space
    gl_Position = projectionMatrix * viewMatrix * worldPos;
}
