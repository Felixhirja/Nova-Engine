#pragma once

#include <vector>
#include <cstdint>

#ifdef _WIN32
#include <windows.h>
#endif

#include <glad/glad.h>

struct MeshVertex {
    GLfloat px;
    GLfloat py;
    GLfloat pz;
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
    GLfloat u;
    GLfloat v;

    MeshVertex()
        : px(0.0f), py(0.0f), pz(0.0f), r(1.0f), g(1.0f), b(1.0f), a(1.0f), u(0.0f), v(0.0f) {}

    MeshVertex(GLfloat x, GLfloat y, GLfloat z,
               GLfloat red = 1.0f, GLfloat green = 1.0f, GLfloat blue = 1.0f, GLfloat alpha = 1.0f,
               GLfloat texU = 0.0f, GLfloat texV = 0.0f)
        : px(x), py(y), pz(z), r(red), g(green), b(blue), a(alpha), u(texU), v(texV) {}
};

enum MeshAttribute : uint32_t {
    MeshAttribute_Position = 1u << 0,
    MeshAttribute_Color    = 1u << 1,
    MeshAttribute_TexCoord = 1u << 2
};

class Mesh {
public:
    Mesh();
    Mesh(GLenum drawMode, std::vector<MeshVertex> vertices, std::vector<GLuint> indices = {},
         uint32_t attributes = MeshAttribute_Position | MeshAttribute_Color);

    void Draw() const;
    bool Empty() const { return vertices_.empty(); }
    void Clear();

    GLenum DrawMode() const { return drawMode_; }
    const std::vector<MeshVertex>& Vertices() const { return vertices_; }
    const std::vector<GLuint>& Indices() const { return indices_; }
    uint32_t Attributes() const { return attributes_; }

    std::vector<MeshVertex>& MutableVertices() { return vertices_; }
    std::vector<GLuint>& MutableIndices() { return indices_; }
    void SetDrawMode(GLenum mode) { drawMode_ = mode; }
    void SetAttributes(uint32_t attributes);

private:
    GLenum drawMode_;
    std::vector<MeshVertex> vertices_;
    std::vector<GLuint> indices_;
    uint32_t attributes_;
};

class MeshBuilder {
public:
    explicit MeshBuilder(GLenum drawMode);

    void ReserveVertices(std::size_t count);
    void ReserveIndices(std::size_t count);

    std::size_t VertexCount() const { return vertices_.size(); }
    std::size_t IndexCount() const { return indices_.size(); }

    GLuint CurrentIndex() const { return static_cast<GLuint>(vertices_.size()); }

    void AddVertex(const MeshVertex& vertex);
    void AddVertex(GLfloat x, GLfloat y, GLfloat z,
                   GLfloat r = 1.0f, GLfloat g = 1.0f, GLfloat b = 1.0f, GLfloat a = 1.0f,
                   GLfloat u = 0.0f, GLfloat v = 0.0f);

    void AddTriangle(GLuint a, GLuint b, GLuint c);
    void AddQuad(GLuint a, GLuint b, GLuint c, GLuint d);
    void AddLine(GLuint a, GLuint b);

    std::vector<MeshVertex>& Vertices() { return vertices_; }
    std::vector<GLuint>& Indices() { return indices_; }

    Mesh Build(bool useIndices = true);
    void Reset();

private:
    void UpdateAttributeFlags(const MeshVertex& v);

    GLenum drawMode_;
    std::vector<MeshVertex> vertices_;
    std::vector<GLuint> indices_;
    uint32_t attributes_;
};
