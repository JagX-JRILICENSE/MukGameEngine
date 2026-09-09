#include "Mesh.h"

namespace Muk {

Mesh Mesh::CreateTriangle() {
    Mesh mesh;
    mesh.Vertices = {
        { { 0.0f,  0.5f, 0.0f}, {0,0,1}, {0.5f, 1.0f}, {1,0,0,1} },
        { {-0.5f, -0.5f, 0.0f}, {0,0,1}, {0.0f, 0.0f}, {0,1,0,1} },
        { { 0.5f, -0.5f, 0.0f}, {0,0,1}, {1.0f, 0.0f}, {0,0,1,1} },
    };
    mesh.Indices = { 0, 1, 2 };
    return mesh;
}

Mesh Mesh::CreateQuad() {
    Mesh mesh;
    mesh.Vertices = {
        { {-0.5f, -0.5f, 0.0f}, {0,0,1}, {0,0}, {1,1,1,1} },
        { { 0.5f, -0.5f, 0.0f}, {0,0,1}, {1,0}, {1,1,1,1} },
        { { 0.5f,  0.5f, 0.0f}, {0,0,1}, {1,1}, {1,1,1,1} },
        { {-0.5f,  0.5f, 0.0f}, {0,0,1}, {0,1}, {1,1,1,1} },
    };
    mesh.Indices = { 0, 1, 2, 2, 3, 0 };
    return mesh;
}

Mesh Mesh::CreateCube() {
    Mesh mesh;
    const Vec3 positions[8] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f},
        { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}
    };
    for (int i = 0; i < 8; ++i) {
        Vertex v;
        v.Position = positions[i];
        v.Normal = v.Position.Normalized();
        v.TexCoord = {0, 0};
        v.Color = { (positions[i].x + 0.5f), (positions[i].y + 0.5f), (positions[i].z + 0.5f), 1.0f };
        mesh.Vertices.push_back(v);
    }
    mesh.Indices = {
        4,5,6, 6,7,4,
        1,0,3, 3,2,1,
        0,4,7, 7,3,0,
        5,1,2, 2,6,5,
        3,7,6, 6,2,3,
        0,1,5, 5,4,0
    };
    return mesh;
}

} // namespace Muk
