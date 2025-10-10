#version 110

// Skybox fragment shader
// Samples from a cubemap texture

varying vec3 TexCoords;

uniform samplerCube skybox;

void main() {
    gl_FragColor = textureCube(skybox, TexCoords);
}
