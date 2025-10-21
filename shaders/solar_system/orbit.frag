#version 110

// Orbit visualization fragment shader
// Applies configurable opacity to orbit colors

varying vec4 vColor;

uniform float opacityMultiplier;

void main() {
    float alpha = clamp(vColor.a * opacityMultiplier, 0.0, 1.0);
    gl_FragColor = vec4(vColor.rgb, alpha);
}
