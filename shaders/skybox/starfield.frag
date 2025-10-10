#version 110

// Procedural starfield shader
// Generates stars in 3D space without textures

varying vec3 TexCoords;

uniform float time;
uniform float starDensity;  // Default: 0.002
uniform float starBrightness;  // Default: 1.0

// Simple hash function for pseudo-random star positions
float hash(vec3 p) {
    p = fract(p * vec3(443.897, 441.423, 437.195));
    p += dot(p, p.yzx + 19.19);
    return fract((p.x + p.y) * p.z);
}

void main() {
    // Sample 3D grid for star positions
    vec3 coord = normalize(TexCoords) * 100.0;
    vec3 gridCell = floor(coord);
    
    // Check if this cell contains a star
    float starRandom = hash(gridCell);
    
    // Background space color (very dark blue-black)
    vec3 spaceColor = vec3(0.01, 0.01, 0.02);
    
    // Create star if random value exceeds threshold
    if (starRandom > (1.0 - starDensity)) {
        // Star position within cell
        vec3 starPos = gridCell + hash(gridCell + vec3(1.0, 2.0, 3.0)) * 0.5;
        
        // Distance to star
        float dist = length(coord - starPos);
        
        // Star brightness falloff
        float brightness = 1.0 / (1.0 + dist * dist * 0.5);
        brightness = pow(brightness, 3.0) * starBrightness;
        
        // Star color variation (white to blue)
        float colorVariation = hash(gridCell + vec3(4.0, 5.0, 6.0));
        vec3 starColor = mix(vec3(1.0, 1.0, 1.0), vec3(0.8, 0.9, 1.0), colorVariation);
        
        // Twinkle effect (optional, based on time)
        float twinkle = 0.8 + 0.2 * sin(time + hash(gridCell) * 6.28);
        brightness *= twinkle;
        
        // Combine star with background
        vec3 finalColor = mix(spaceColor, starColor, brightness);
        gl_FragColor = vec4(finalColor, 1.0);
    } else {
        // Just background
        gl_FragColor = vec4(spaceColor, 1.0);
    }
}
