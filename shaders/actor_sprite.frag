#version 330 core

in vec2 vTexCoord;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D textureSampler;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(textureSampler, vTexCoord);
        FragColor = texColor * vColor;
    } else {
        FragColor = vColor;
    }
}