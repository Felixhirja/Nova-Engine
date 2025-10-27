#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vWorldPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vec4 worldPos = model * vec4(aPosition, 1.0);
    gl_Position = projection * view * worldPos;

    vWorldPos = worldPos.xyz;
    vNormal = normalize(mat3(model) * aNormal);
    vTexCoord = aTexCoord;
}