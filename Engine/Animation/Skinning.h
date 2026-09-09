#pragma once

#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "Skeleton.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include <vector>
#include <string>

namespace Muk {

class Renderer;

/**
 * Applies skin matrices to a bind-pose mesh on CPU (works without shader changes).
 * For demo: each bone draws a small cube at skinned position = GPU path preview.
 * Full vertex skinning uses the same matrices uploaded to a future skin CB.
 */
class SkinningSystem {
public:
    static constexpr int MaxBones = 64;

    /** Draw bone cubes for an Animator using SkinMatrices */
    static void DrawSkeletonDebug(Renderer& renderer, const Animator& anim,
                                  const Mat4& world, const Material& mat);

    /** Skin a single bind position by up to 4 influences (weights sum ~1) */
    static Vec3 SkinPoint(const Vec3& bindPos, const int boneIdx[4], const float weights[4],
                          const std::vector<Mat4>& skinMats);

    /** Build a simple skinned arm mesh vertices from demo skeleton pose */
    static Mesh BuildSkinnedArmMesh(const Animator& anim);
};

} // namespace Muk
