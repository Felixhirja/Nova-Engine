#version 330 core

in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vWorldPos;

out vec4 FragColor;

uniform vec3 tintColor;
uniform float opacity;
uniform bool useTexture;
uniform sampler2D textureSampler;

// Simple lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;

void main() {
    vec3 normal = normalize(vNormal);

    // Simple diffuse lighting
    float diff = max(dot(normal, -lightDir), 0.0);
    vec3 lighting = ambientColor + lightColor * diff;

    vec4 baseColor;
    if (useTexture) {
        baseColor = texture(textureSampler, vTexCoord);
    } else {
        baseColor = vec4(1.0, 1.0, 1.0, 1.0);
    }

    // Apply tint and opacity
    FragColor = vec4(baseColor.rgb * tintColor * lighting, baseColor.a * opacity);
}