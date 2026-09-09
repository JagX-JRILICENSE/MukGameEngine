#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <vector>
#include <string>

namespace Muk {

struct Vertex {
    Vec3 Position;
    Vec3 Normal;
    Vec2 TexCoord;
    Vec4 Color{1,1,1,1};
    // aliases used by some loaders
    Vec2& UV() { return TexCoord; }
};

class Mesh {
public:
    Mesh() = default;

    std::string Name;
    // Public for asset loaders / skinning helpers
    std::vector<Vertex> Vertices;
    std::vector<u32> Indices;

    void SetVertices(const std::vector<Vertex>& vertices) { Vertices = vertices; }
    void SetIndices(const std::vector<u32>& indices) { Indices = indices; }

    const std::vector<Vertex>& GetVertices() const { return Vertices; }
    const std::vector<u32>& GetIndices() const { return Indices; }
    u32 GetIndexCount() const { return static_cast<u32>(Indices.size()); }
    u32 GetVertexCount() const { return static_cast<u32>(Vertices.size()); }

    static Mesh CreateTriangle();
    static Mesh CreateCube();
    static Mesh CreateQuad();
};

} // namespace Muk
