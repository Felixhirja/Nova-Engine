#version 110

// Orbit visualization vertex shader
// Renders colored line segments for orbital paths

attribute vec3 aPos;
attribute vec4 aColor;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec4 vColor;

void main() {
    vColor = aColor;
    gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
}
