#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <vector>

namespace Muk {

struct Vertex {
    Vec3 Position;
    Vec3 Normal;
    Vec2 TexCoord;
    Vec4 Color;
};

class Mesh {
public:
    Mesh() = default;

    void SetVertices(const std::vector<Vertex>& vertices);
    void SetIndices(const std::vector<u32>& indices);

    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    const std::vector<u32>& GetIndices() const { return m_Indices; }
    u32 GetIndexCount() const { return static_cast<u32>(m_Indices.size()); }
    u32 GetVertexCount() const { return static_cast<u32>(m_Vertices.size()); }

    // Built-in primitives
    static Mesh CreateTriangle();
    static Mesh CreateCube();
    static Mesh CreateQuad();

private:
    std::vector<Vertex> m_Vertices;
    std::vector<u32> m_Indices;
};

} // namespace Muk
