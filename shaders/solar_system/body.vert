#version 110

// Solar system body vertex shader
// Applies model/view/projection transforms and forwards normals and colors

attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec2 aTexCoord;
attribute vec4 aColor;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

varying vec3 vFragPos;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec4 vColor;

void main() {
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal = mat3(modelMatrix) * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = projectionMatrix * viewMatrix * worldPos;
}
