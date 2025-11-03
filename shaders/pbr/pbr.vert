#version 430 core

layout (location = 0) in vec3 a_Position;
layout (location = 1) in vec3 a_Normal;
layout (location = 2) in vec2 a_TexCoord;
layout (location = 3) in vec3 a_Tangent;
layout (location = 4) in vec3 a_Bitangent;

// Uniforms
uniform mat4 u_Model;
uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_ViewProjection;

// Outputs to fragment shader
out vec3 v_WorldPos;
out vec3 v_Normal;
out vec2 v_TexCoord;
out mat3 v_TBN;

void main() {
    // Transform vertex position to world space
    vec4 worldPos = u_Model * vec4(a_Position, 1.0);
    v_WorldPos = worldPos.xyz;
    
    // Transform normal to world space
    mat3 normalMatrix = transpose(inverse(mat3(u_Model)));
    v_Normal = normalize(normalMatrix * a_Normal);
    
    // Pass through texture coordinates
    v_TexCoord = a_TexCoord;
    
    // Construct TBN matrix for normal mapping
    vec3 T = normalize(normalMatrix * a_Tangent);
    vec3 B = normalize(normalMatrix * a_Bitangent);
    vec3 N = v_Normal;
    
    // Gram-Schmidt orthogonalization
    T = normalize(T - dot(T, N) * N);
    B = cross(N, T);
    
    v_TBN = mat3(T, B, N);
    
    // Final position
    gl_Position = u_ViewProjection * worldPos;
}
