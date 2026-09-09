#include "Skinning.h"
#include "Renderer/Renderer.h"

namespace Muk {

Vec3 SkinningSystem::SkinPoint(const Vec3& bindPos, const int boneIdx[4], const float weights[4],
                               const std::vector<Mat4>& skinMats) {
    Vec3 out{};
    float wsum = 0;
    for (int i = 0; i < 4; ++i) {
        int b = boneIdx[i];
        float w = weights[i];
        if (b < 0 || b >= (int)skinMats.size() || w <= 0) continue;
        // Transform point by skin matrix (column-major)
        const Mat4& m = skinMats[b];
        float x = bindPos.x, y = bindPos.y, z = bindPos.z;
        Vec3 p{
            m.m[0]*x + m.m[4]*y + m.m[8]*z + m.m[12],
            m.m[1]*x + m.m[5]*y + m.m[9]*z + m.m[13],
            m.m[2]*x + m.m[6]*y + m.m[10]*z + m.m[14]
        };
        out.x += p.x * w; out.y += p.y * w; out.z += p.z * w;
        wsum += w;
    }
    if (wsum > 1e-5f) { out.x /= wsum; out.y /= wsum; out.z /= wsum; }
    else return bindPos;
    return out;
}

void SkinningSystem::DrawSkeletonDebug(Renderer& renderer, const Animator& anim,
                                       const Mat4& world, const Material& mat) {
    for (size_t i = 0; i < anim.SkinMatrices.size(); ++i) {
        Mat4 boneWorld = world * anim.SkinMatrices[i];
        Mat4 cube = boneWorld * Mat4::Scale({ 0.12f, 0.12f, 0.12f });
        renderer.DrawMesh("Cube", cube, mat);
    }
}

Mesh SkinningSystem::BuildSkinnedArmMesh(const Animator& anim) {
    Mesh mesh;
    mesh.Name = "SkinnedArm";
    // Simple line of cubes along bone chain — positions from skin matrices translation
    for (size_t i = 0; i < anim.SkinMatrices.size(); ++i) {
        const Mat4& m = anim.SkinMatrices[i];
        Vec3 p{ m.m[12], m.m[13], m.m[14] };
        // unit cube verts offset
        const float s = 0.1f;
        Vec3 corners[8] = {
            {p.x-s,p.y-s,p.z-s},{p.x+s,p.y-s,p.z-s},{p.x+s,p.y+s,p.z-s},{p.x-s,p.y+s,p.z-s},
            {p.x-s,p.y-s,p.z+s},{p.x+s,p.y-s,p.z+s},{p.x+s,p.y+s,p.z+s},{p.x-s,p.y+s,p.z+s}
        };
        u32 base = (u32)mesh.Vertices.size();
        for (auto& c : corners) {
            Vertex v; v.Position = c; v.Normal = {0,1,0}; v.UV = {0,0};
            mesh.Vertices.push_back(v);
        }
        const u32 idx[] = {0,1,2,0,2,3, 4,6,5,4,7,6, 0,4,5,0,5,1, 2,6,7,2,7,3, 0,3,7,0,7,4, 1,5,6,1,6,2};
        for (u32 id : idx) mesh.Indices.push_back(base + id);
    }
    return mesh;
}

} // namespace Muk
