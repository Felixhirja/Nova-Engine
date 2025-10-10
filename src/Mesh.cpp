#include "Mesh.h"

#include <algorithm>

Mesh::Mesh()
    : drawMode_(GL_TRIANGLES), attributes_(MeshAttribute_Position | MeshAttribute_Color) {
}

Mesh::Mesh(GLenum drawMode, std::vector<MeshVertex> vertices, std::vector<GLuint> indices, uint32_t attributes)
    : drawMode_(drawMode), vertices_(std::move(vertices)), indices_(std::move(indices)), attributes_(attributes) {
    if (attributes_ == 0u) {
        attributes_ = MeshAttribute_Position;
    }
}

void Mesh::Draw() const {
    if (vertices_.empty()) {
        return;
    }

    glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<const GLvoid*>(&vertices_[0].px));

    if (attributes_ & MeshAttribute_Color) {
        glEnableClientState(GL_COLOR_ARRAY);
        glColorPointer(4, GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<const GLvoid*>(&vertices_[0].r));
    } else {
        glDisableClientState(GL_COLOR_ARRAY);
    }

    if ((attributes_ & MeshAttribute_TexCoord) != 0u) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof(MeshVertex), reinterpret_cast<const GLvoid*>(&vertices_[0].u));
    } else {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }

    if (!indices_.empty()) {
        glDrawElements(drawMode_, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, indices_.data());
    } else {
        glDrawArrays(drawMode_, 0, static_cast<GLsizei>(vertices_.size()));
    }

    glPopClientAttrib();
}

void Mesh::Clear() {
    vertices_.clear();
    indices_.clear();
}

void Mesh::SetAttributes(uint32_t attributes) {
    attributes_ = attributes == 0u ? MeshAttribute_Position : attributes;
}

MeshBuilder::MeshBuilder(GLenum drawMode)
    : drawMode_(drawMode), attributes_(0u) {
}

void MeshBuilder::ReserveVertices(std::size_t count) {
    vertices_.reserve(count);
}

void MeshBuilder::ReserveIndices(std::size_t count) {
    indices_.reserve(count);
}

void MeshBuilder::AddVertex(const MeshVertex& vertex) {
    vertices_.push_back(vertex);
    UpdateAttributeFlags(vertex);
}

void MeshBuilder::AddVertex(GLfloat x, GLfloat y, GLfloat z,
                            GLfloat r, GLfloat g, GLfloat b, GLfloat a,
                            GLfloat u, GLfloat v) {
    MeshVertex vtx{x, y, z, r, g, b, a, u, v};
    AddVertex(vtx);
}

void MeshBuilder::AddTriangle(GLuint a, GLuint b, GLuint c) {
    indices_.push_back(a);
    indices_.push_back(b);
    indices_.push_back(c);
}

void MeshBuilder::AddQuad(GLuint a, GLuint b, GLuint c, GLuint d) {
    // Two triangles (a,b,c) and (a,c,d)
    indices_.push_back(a);
    indices_.push_back(b);
    indices_.push_back(c);
    indices_.push_back(a);
    indices_.push_back(c);
    indices_.push_back(d);
}

void MeshBuilder::AddLine(GLuint a, GLuint b) {
    indices_.push_back(a);
    indices_.push_back(b);
}

Mesh MeshBuilder::Build(bool useIndices) {
    uint32_t attrib = attributes_;
    if (attrib == 0u) {
        attrib = MeshAttribute_Position;
    }
    std::vector<MeshVertex> vertices = std::move(vertices_);
    std::vector<GLuint> indices;
    if (useIndices) {
        indices = std::move(indices_);
    } else {
        indices_.clear();
    }

    // reset builder for potential reuse
    attributes_ = 0u;

    return Mesh(drawMode_, std::move(vertices), std::move(indices), attrib);
}

void MeshBuilder::Reset() {
    vertices_.clear();
    indices_.clear();
    attributes_ = 0u;
}

void MeshBuilder::UpdateAttributeFlags(const MeshVertex& v) {
    attributes_ |= MeshAttribute_Position;

    // Always treat color as available (compatibility with existing rendering expectations)
    attributes_ |= MeshAttribute_Color;

    if (v.u != 0.0f || v.v != 0.0f) {
        attributes_ |= MeshAttribute_TexCoord;
    }
}
