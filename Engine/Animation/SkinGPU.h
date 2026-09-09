#pragma once

#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Skeleton.h"
#include <vector>
#include <cstring>

namespace Muk {

/** Matches HLSL cbuffer SkinCB : register(b1) */
struct alignas(16) SkinCBData {
    float Bones[64 * 16]; // 64 float4x4 column-major
    int BoneCount = 0;
    int _pad[3] = {};
};

inline void FillSkinCB(SkinCBData& cb, const Animator& anim) {
    std::memset(&cb, 0, sizeof(cb));
    int n = (int)std::min(anim.SkinMatrices.size(), size_t(64));
    cb.BoneCount = n;
    for (int i = 0; i < n; ++i) {
        const Mat4& m = anim.SkinMatrices[i];
        std::memcpy(&cb.Bones[i * 16], m.m, sizeof(float) * 16);
    }
}

/**
 * Vertex layout for skinned meshes (CPU mirror of shader input).
 * POSITION, NORMAL, TEXCOORD, BLENDINDICES, BLENDWEIGHT
 */
struct SkinnedVertex {
    float Position[3];
    float Normal[3];
    float UV[2];
    uint32_t BoneIndices[4];
    float BoneWeights[4];
};

} // namespace Muk
