#pragma once
#include "Math/Vector.h"
#include "Renderer/Mesh.h"
#include <vector>
#include <cmath>
#include <string>

namespace Muk {

class Heightfield {
public:
    void Generate(int res, float size, float heightAmp = 2.f) {
        m_Res = res; m_Size = size; m_Amp = heightAmp;
        m_Heights.assign(res * res, 0.f);
        for (int z = 0; z < res; ++z)
            for (int x = 0; x < res; ++x) {
                float u = (float)x / (res - 1), v = (float)z / (res - 1);
                float h = std::sin(u * 6.28f) * std::cos(v * 4.0f) * heightAmp * 0.5f;
                h += std::sin(u * 12.f + v * 8.f) * heightAmp * 0.15f;
                m_Heights[z * res + x] = h;
            }
    }

    float Sample(int x, int z) const {
        x = std::max(0, std::min(m_Res - 1, x));
        z = std::max(0, std::min(m_Res - 1, z));
        return m_Heights[z * m_Res + x];
    }

    Mesh BuildMesh() const {
        Mesh mesh; mesh.Name = "Terrain";
        float step = m_Size / (m_Res - 1);
        float origin = -m_Size * 0.5f;
        for (int z = 0; z < m_Res; ++z)
            for (int x = 0; x < m_Res; ++x) {
                Vertex v;
                v.Position = { origin + x * step, Sample(x, z), origin + z * step };
                v.Normal = { 0, 1, 0 };
                v.UV = { (float)x / (m_Res - 1), (float)z / (m_Res - 1) };
                mesh.Vertices.push_back(v);
            }
        for (int z = 0; z < m_Res - 1; ++z)
            for (int x = 0; x < m_Res - 1; ++x) {
                u32 i = z * m_Res + x;
                mesh.Indices.push_back(i);
                mesh.Indices.push_back(i + m_Res);
                mesh.Indices.push_back(i + 1);
                mesh.Indices.push_back(i + 1);
                mesh.Indices.push_back(i + m_Res);
                mesh.Indices.push_back(i + m_Res + 1);
            }
        return mesh;
    }

    int Resolution() const { return m_Res; }

private:
    int m_Res = 0;
    float m_Size = 32.f, m_Amp = 2.f;
    std::vector<float> m_Heights;
};

struct WaterPlane {
    float Height = 0.15f;
    Vec3 Color{ 0.15f, 0.35f, 0.55f };
    float Opacity = 0.65f;
    float WaveSpeed = 1.2f;
    float Time = 0;

    void Update(float dt) { Time += dt * WaveSpeed; }

    Mat4 WorldMatrix(float size = 40.f) const {
        float bob = std::sin(Time) * 0.05f;
        return Mat4::Translation({ 0, Height + bob, 0 }) * Mat4::Scale({ size, 0.05f, size });
    }
};

} // namespace Muk
