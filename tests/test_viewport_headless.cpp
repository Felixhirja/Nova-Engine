#include "Viewport3D.h"
#include "Transform.h"
#include <iostream>

int main() {
    // Construct viewport but do not call Init() so this runs safely in headless CI
    Viewport3D viewport;

    // Build a mesh using the renderer's factory (no GL required)
    Mesh playerMesh = Viewport3D::CreatePlayerAvatarMesh();

    // Bind the mesh to an arbitrary entity id and exercise draw paths.
    const Entity testEntity = 1;
    viewport.SetEntityMesh(testEntity, std::move(playerMesh), 1.0f);

    // Create a simple transform and draw the entity. When no GL backend is active
    // the renderer falls back to an ASCII output path which is safe for headless runs.
    Transform t;
    t.x = 0.0;
    t.y = 0.0;
    t.z = 0.0;

    // Draw by entity (uses the override mesh) and by transform (generic path)
    viewport.DrawEntity(testEntity, t);
    viewport.DrawEntity(t);

    // Clear bindings and shutdown (no-op for ASCII path)
    viewport.ClearEntityMesh(testEntity);
    viewport.ClearEntityMeshes();
    viewport.Shutdown();

    std::cout << "Viewport headless smoke test completed" << std::endl;
    return 0;
}
