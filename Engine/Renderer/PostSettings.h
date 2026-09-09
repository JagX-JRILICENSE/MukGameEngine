#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"

namespace Muk {

/** Global look / environment — uploaded to frame CB / used by lit shader */
struct PostSettings {
    f32 Exposure = 1.15f;
    f32 BloomThreshold = 1.0f;
    f32 BloomStrength = 0.45f;
    f32 BloomRadius = 1.5f; // soft spread factor for cheap bloom tint
    bool TonemapACES = true;
    bool BloomEnabled = true;

    // Hemisphere IBL ambient
    Vec3 SkyColor{0.42f, 0.55f, 0.82f};
    Vec3 GroundColor{0.14f, 0.12f, 0.10f};
    f32 IblStrength = 0.35f;

    // Specular-ish env tint for "cubemap IBL" approximation
    Vec3 EnvSpecular{0.55f, 0.60f, 0.75f};
    f32 EnvSpecularStrength = 0.2f;

    // Computed ambient for a normal (hemisphere blend)
    Vec3 SampleHemisphere(const Vec3& normal) const {
        float t = normal.y * 0.5f + 0.5f;
        Vec3 amb{
            SkyColor.x * t + GroundColor.x * (1.0f - t),
            SkyColor.y * t + GroundColor.y * (1.0f - t),
            SkyColor.z * t + GroundColor.z * (1.0f - t)
        };
        return { amb.x * IblStrength, amb.y * IblStrength, amb.z * IblStrength };
    }
};

} // namespace Muk
