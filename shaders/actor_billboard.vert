#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

out vec2 vTexCoord;
out vec4 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraRight;
uniform vec3 cameraUp;

void main() {
    // Billboard: rotate quad to face camera
    vec3 worldPos = (model * vec4(0.0, 0.0, 0.0, 1.0)).xyz;
    vec3 vertexPos = aPosition.x * cameraRight + aPosition.y * cameraUp;
    worldPos += vertexPos;

    gl_Position = projection * view * vec4(worldPos, 1.0);
    vTexCoord = aTexCoord;
    vColor = aColor;
}