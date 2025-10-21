#version 110

// Star fragment shader for emissive solar bodies
// Uses rim lighting to create a soft glow around the star

varying vec3 vFragPos;
varying vec3 vNormal;
varying vec4 vColor;

uniform vec3 baseColor;
uniform vec3 viewPos;
uniform float surfaceIntensity;
uniform float glowStrength;
uniform float rimExponent;
uniform float ambientStrength;

void main() {
    vec3 color = baseColor;
    if (vColor.a > 0.0) {
        color *= vColor.rgb;
    }

    vec3 normal = normalize(vNormal);
    vec3 viewDir = normalize(viewPos - vFragPos);
    float rim = pow(1.0 - max(dot(normal, viewDir), 0.0), rimExponent);

    float intensity = surfaceIntensity + ambientStrength + glowStrength * rim;
    gl_FragColor = vec4(color * intensity, 1.0);
}
