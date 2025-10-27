#define PRIMITIVEMESH_FORCE_NO_GL
#include "graphics/PrimitiveMesh.h"
#include <iostream>
#include <vector>

int main() {
    PrimitiveMesh mesh;

    // Simple colored triangle: x,y,z, r,g,b
    std::vector<float> verts = {
        0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,
       -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,
        0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f
    };
    std::vector<unsigned int> idx = {0,1,2};

    mesh.Upload(verts, idx, static_cast<int>(6 * sizeof(float)), true);
    mesh.Draw();
    mesh.Cleanup();

    std::cout << "PrimitiveMesh test completed" << std::endl;
    return 0;
}
