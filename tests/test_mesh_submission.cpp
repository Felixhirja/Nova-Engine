#include "graphics/MeshSubmission.h"
#include "graphics/PrimitiveMesh.h"
#include "Mesh.h"

int main() {
    MeshBuilder builder(0x0004u); // GL_TRIANGLES equivalent
    builder.AddVertex(-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
    builder.AddVertex(0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    builder.AddVertex(0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f, 1.0f);
    builder.AddTriangle(0, 1, 2);

    Mesh mesh = builder.Build(true);
    mesh.SetAttributes(MeshAttribute_Position | MeshAttribute_Color | MeshAttribute_TexCoord);

    MeshSubmission triangleSubmission = MeshSubmissionBuilder::FromMesh(mesh);
    PrimitiveMesh trianglePrimitive;
    trianglePrimitive.Upload(triangleSubmission);
    trianglePrimitive.Draw();
    trianglePrimitive.Cleanup();

    SpriteQuadDescriptor quadDesc;
    quadDesc.width = 2.0f;
    quadDesc.height = 1.0f;
    quadDesc.depth = -0.25f;
    quadDesc.u0 = 0.0f;
    quadDesc.v0 = 0.0f;
    quadDesc.u1 = 0.5f;
    quadDesc.v1 = 0.5f;
    quadDesc.color = {0.6f, 0.7f, 1.0f, 0.9f};

    MeshSubmission quadSubmission = MeshSubmissionBuilder::SpriteQuad(quadDesc);
    PrimitiveMesh quadPrimitive;
    quadPrimitive.Upload(quadSubmission);
    quadPrimitive.Draw();
    quadPrimitive.Cleanup();

    SpriteSheetDescriptor sheetDesc;
    sheetDesc.frameWidth = 64;
    sheetDesc.frameHeight = 48;
    sheetDesc.frameCount = 8;
    sheetDesc.columns = 4;
    sheetDesc.frameIndex = 5;
    sheetDesc.pixelsPerUnit = 32.0f;
    sheetDesc.depth = 0.5f;
    sheetDesc.color = {1.0f, 0.8f, 0.2f, 1.0f};

    MeshSubmission frameSubmission = MeshSubmissionBuilder::SpriteFrame(sheetDesc);
    PrimitiveMesh framePrimitive;
    framePrimitive.Upload(frameSubmission);
    framePrimitive.Draw();
    framePrimitive.Cleanup();

    return 0;
}
