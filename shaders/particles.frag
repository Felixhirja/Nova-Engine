#version 460 core

// Input from vertex shader
in vec4 vColor;

// Output color
out vec4 FragColor;

void main() {
    // gl_PointCoord gives us texture coordinates for the point sprite (0,0 to 1,1)
    vec2 coord = gl_PointCoord * 2.0 - 1.0;  // Map to -1..1 range
    
    // Calculate distance from center
    float dist = length(coord);
    
    // Create circular particle with soft falloff
    if (dist > 1.0) {
        discard;  // Outside circle
    }
    
    // Smooth falloff from center to edge
    float alpha = 1.0 - smoothstep(0.0, 1.0, dist);
    alpha *= alpha;  // Square for sharper falloff
    
    // Apply alpha to color
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
