#include "Mesh.h"

namespace Muk {

void Mesh::SetVertices(const std::vector<Vertex>& vertices) {
    m_Vertices = vertices;
}

void Mesh::SetIndices(const std::vector<u32>& indices) {
    m_Indices = indices;
}

Mesh Mesh::CreateTriangle() {
    Mesh mesh;
    mesh.m_Vertices = {
        { { 0.0f,  0.5f, 0.0f}, {0,0,1}, {0.5f, 1.0f}, {1,0,0,1} }, // top - red
        { {-0.5f, -0.5f, 0.0f}, {0,0,1}, {0.0f, 0.0f}, {0,1,0,1} }, // bottom left - green
        { { 0.5f, -0.5f, 0.0f}, {0,0,1}, {1.0f, 0.0f}, {0,0,1,1} }, // bottom right - blue
    };
    mesh.m_Indices = { 0, 1, 2 };
    return mesh;
}

Mesh Mesh::CreateQuad() {
    Mesh mesh;
    mesh.m_Vertices = {
        { {-0.5f, -0.5f, 0.0f}, {0,0,1}, {0,0}, {1,1,1,1} },
        { { 0.5f, -0.5f, 0.0f}, {0,0,1}, {1,0}, {1,1,1,1} },
        { { 0.5f,  0.5f, 0.0f}, {0,0,1}, {1,1}, {1,1,1,1} },
        { {-0.5f,  0.5f, 0.0f}, {0,0,1}, {0,1}, {1,1,1,1} },
    };
    mesh.m_Indices = { 0, 1, 2, 2, 3, 0 };
    return mesh;
}

Mesh Mesh::CreateCube() {
    Mesh mesh;
    // 8 vertices of a unit cube centered at origin
    const Vec3 positions[8] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
        { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
    };

    // Simple colored cube (per-vertex color by position)
    for (int i = 0; i < 8; ++i) {
        Vertex v;
        v.Position = positions[i];
        v.Normal = v.Position.Normalized();
        v.TexCoord = {0, 0};
        v.Color = { (positions[i].x + 0.5f), (positions[i].y + 0.5f), (positions[i].z + 0.5f), 1.0f };
        mesh.m_Vertices.push_back(v);
    }

    // 12 triangles (2 per face)
    mesh.m_Indices = {
        // Front
        4,5,6, 6,7,4,
        // Back
        1,0,3, 3,2,1,
        // Left
        0,4,7, 7,3,0,
        // Right
        5,1,2, 2,6,5,
        // Top
        3,7,6, 6,2,3,
        // Bottom
        0,1,5, 5,4,0
    };

    return mesh;
}

} // namespace Muk
