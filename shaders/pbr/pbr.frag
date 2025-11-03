#version 430 core

// Material texture inputs
uniform sampler2D u_AlbedoMap;
uniform sampler2D u_NormalMap;
uniform sampler2D u_MetallicRoughnessMap; // R=unused, G=roughness, B=metallic (GLTF standard)
uniform sampler2D u_AOMap;
uniform sampler2D u_EmissiveMap;

// Material parameters
uniform vec3 u_BaseColor;
uniform float u_Roughness;
uniform float u_Metalness;
uniform vec3 u_Emissive;
uniform float u_Alpha;

// Texture flags
uniform bool u_HasAlbedoMap;
uniform bool u_HasNormalMap;
uniform bool u_HasMetallicRoughnessMap;
uniform bool u_HasAOMap;
uniform bool u_HasEmissiveMap;

// Lighting
uniform vec3 u_CameraPos;
uniform vec3 u_LightPositions[4];
uniform vec3 u_LightColors[4];
uniform float u_LightIntensities[4];
uniform int u_LightCount;

// Ambient lighting
uniform vec3 u_AmbientColor;
uniform float u_AmbientIntensity;

// Inputs from vertex shader
in vec3 v_WorldPos;
in vec3 v_Normal;
in vec2 v_TexCoord;
in mat3 v_TBN;

// Output
out vec4 FragColor;

const float PI = 3.14159265359;

// Fresnel-Schlick approximation
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Trowbridge-Reitz normal distribution function
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return num / max(denom, 0.0001);
}

// Schlick-GGX geometry function
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / max(denom, 0.0001);
}

// Smith's method for geometry obstruction/shadowing
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

void main() {
    // Sample textures
    vec3 albedo = u_HasAlbedoMap 
        ? pow(texture(u_AlbedoMap, v_TexCoord).rgb, vec3(2.2)) * u_BaseColor
        : u_BaseColor;
    
    // Sample normal map
    vec3 normal = v_Normal;
    if (u_HasNormalMap) {
        vec3 tangentNormal = texture(u_NormalMap, v_TexCoord).rgb * 2.0 - 1.0;
        normal = normalize(v_TBN * tangentNormal);
    }
    
    // Sample metallic-roughness map (GLTF: G=roughness, B=metallic)
    float metallic = u_Metalness;
    float roughness = u_Roughness;
    if (u_HasMetallicRoughnessMap) {
        vec3 mr = texture(u_MetallicRoughnessMap, v_TexCoord).rgb;
        roughness *= mr.g;
        metallic *= mr.b;
    }
    
    // Clamp values to valid range
    roughness = clamp(roughness, 0.04, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);
    
    // Sample ambient occlusion
    float ao = u_HasAOMap ? texture(u_AOMap, v_TexCoord).r : 1.0;
    
    // Sample emissive
    vec3 emissive = u_HasEmissiveMap 
        ? texture(u_EmissiveMap, v_TexCoord).rgb * u_Emissive
        : u_Emissive;
    
    // Calculate reflectance at normal incidence
    // For dielectrics (non-metals), F0 is typically 0.04
    // For metals, F0 is the albedo color
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);
    
    // View direction
    vec3 V = normalize(u_CameraPos - v_WorldPos);
    
    // Reflectance equation
    vec3 Lo = vec3(0.0);
    
    for(int i = 0; i < u_LightCount; ++i) {
        // Per-light radiance
        vec3 L = normalize(u_LightPositions[i] - v_WorldPos);
        vec3 H = normalize(V + L);
        float distance = length(u_LightPositions[i] - v_WorldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = u_LightColors[i] * u_LightIntensities[i] * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(normal, H, roughness);
        float G = GeometrySmith(normal, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // Specular contribution
        vec3 kS = F;
        
        // Diffuse contribution (energy conservation: kD + kS = 1)
        vec3 kD = vec3(1.0) - kS;
        
        // Metals don't have diffuse reflection
        kD *= 1.0 - metallic;
        
        // Calculate specular term
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(normal, V), 0.0) * max(dot(normal, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        // Add to outgoing radiance Lo
        float NdotL = max(dot(normal, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // Ambient lighting (simplified IBL approximation)
    vec3 ambient = u_AmbientColor * u_AmbientIntensity * albedo * ao;
    
    // Final color
    vec3 color = ambient + Lo + emissive;
    
    // HDR tone mapping (Reinhard)
    color = color / (color + vec3(1.0));
    
    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));
    
    FragColor = vec4(color, u_Alpha);
}
