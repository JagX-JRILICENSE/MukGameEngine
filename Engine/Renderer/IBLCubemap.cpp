#include "IBLCubemap.h"
#include <cmath>
#include <algorithm>

namespace Muk {

static Vec3 FaceDir(int face, float u, float v) {
    // u,v in [-1,1]
    switch (face) {
    case 0: return Vec3{ 1, -v, -u }.Normalized();  // +X
    case 1: return Vec3{ -1, -v, u }.Normalized(); // -X
    case 2: return Vec3{ u, 1, v }.Normalized();   // +Y
    case 3: return Vec3{ u, -1, -v }.Normalized(); // -Y
    case 4: return Vec3{ u, -v, 1 }.Normalized();  // +Z
    default: return Vec3{ -u, -v, -1 }.Normalized(); // -Z
    }
}

void IBLCubemap::Generate(const PostSettings& post) {
    m_Cached = post;
    for (int f = 0; f < 6; ++f) {
        auto tex = std::make_shared<Texture>();
        tex->Width = FaceSize;
        tex->Height = FaceSize;
        tex->Name = "IBL_Face" + std::to_string(f);
        tex->Pixels.resize(FaceSize * FaceSize * 4);
        for (int y = 0; y < FaceSize; ++y) {
            for (int x = 0; x < FaceSize; ++x) {
                float u = (x + 0.5f) / FaceSize * 2.f - 1.f;
                float v = (y + 0.5f) / FaceSize * 2.f - 1.f;
                Vec3 d = FaceDir(f, u, v);
                float t = d.y * 0.5f + 0.5f;
                Vec3 col{
                    post.SkyColor.x * t + post.GroundColor.x * (1 - t),
                    post.SkyColor.y * t + post.GroundColor.y * (1 - t),
                    post.SkyColor.z * t + post.GroundColor.z * (1 - t)
                };
                // Horizon glow
                float horizon = 1.f - std::fabs(d.y);
                col.x += post.EnvSpecular.x * horizon * post.EnvSpecularStrength * 0.3f;
                col.y += post.EnvSpecular.y * horizon * post.EnvSpecularStrength * 0.3f;
                col.z += post.EnvSpecular.z * horizon * post.EnvSpecularStrength * 0.3f;
                int i = (y * FaceSize + x) * 4;
                tex->Pixels[i+0] = (u8)std::clamp(col.x * 255.f, 0.f, 255.f);
                tex->Pixels[i+1] = (u8)std::clamp(col.y * 255.f, 0.f, 255.f);
                tex->Pixels[i+2] = (u8)std::clamp(col.z * 255.f, 0.f, 255.f);
                tex->Pixels[i+3] = 255;
            }
        }
        m_Faces[f] = tex;
    }
    m_Ready = true;
}

Vec3 IBLCubemap::SampleDir(const Vec3& dir) const {
    float t = dir.y * 0.5f + 0.5f;
    return {
        m_Cached.SkyColor.x * t + m_Cached.GroundColor.x * (1 - t),
        m_Cached.SkyColor.y * t + m_Cached.GroundColor.y * (1 - t),
        m_Cached.SkyColor.z * t + m_Cached.GroundColor.z * (1 - t)
    };
}

void BloomExtract::ExtractBright(const std::vector<u8>& rgba, u32 w, u32 h, float threshold,
                                 std::vector<u8>& outBright) {
    outBright.resize(w * h * 4);
    float thr = threshold * 255.f;
    for (u32 i = 0; i < w * h; ++i) {
        float r = rgba[i*4], g = rgba[i*4+1], b = rgba[i*4+2];
        float lum = 0.2126f * r + 0.7152f * g + 0.0722f * b;
        if (lum > thr) {
            outBright[i*4] = (u8)r; outBright[i*4+1] = (u8)g;
            outBright[i*4+2] = (u8)b; outBright[i*4+3] = 255;
        } else {
            outBright[i*4] = outBright[i*4+1] = outBright[i*4+2] = 0;
            outBright[i*4+3] = 255;
        }
    }
}

} // namespace Muk
