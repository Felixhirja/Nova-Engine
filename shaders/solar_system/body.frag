#version 110

// Solar system body fragment shader
// Supports optional textures and vertex colors with Blinn-Phong lighting

varying vec3 vFragPos;
varying vec3 vNormal;
varying vec2 vTexCoord;
varying vec4 vColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;
uniform vec3 baseColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform float emissiveStrength;
uniform int useVertexColor;
uniform int useBaseColor;
uniform int useTexture;
uniform sampler2D albedoMap;

void main() {
    vec3 surfaceColor = (useBaseColor == 1) ? baseColor : vec3(1.0);

    if (useVertexColor == 1) {
        surfaceColor *= vColor.rgb;
    }

    if (useTexture == 1) {
        surfaceColor *= texture2D(albedoMap, vTexCoord).rgb;
    }

    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - vFragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 ambient = ambientStrength * lightColor;
    vec3 lighting = ambient + diffuse + specular;

    vec3 emissive = emissiveStrength * surfaceColor;
    vec3 finalColor = lighting * surfaceColor + emissive;
    float alpha = (useVertexColor == 1) ? vColor.a : 1.0;

    gl_FragColor = vec4(finalColor, clamp(alpha, 0.0, 1.0));
}
