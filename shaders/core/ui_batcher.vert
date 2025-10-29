#version 330 core

layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uProjection;

out vec4 vColor;

void main() {
    vec4 position = vec4(aPosition.xy, 0.0, 1.0);
    gl_Position = uProjection * position;
    vColor = aColor;
}
