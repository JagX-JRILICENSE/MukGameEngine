#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"

namespace Muk {

/** Global look / environment (CPU-side until full IBL cubemap) */
struct PostSettings {
    f32 Exposure = 1.0f;
    f32 BloomThreshold = 1.2f;
    f32 BloomStrength = 0.35f;
    bool TonemapACES = true;

    // Cheap IBL: ambient hemisphere colors
    Vec3 SkyColor{0.45f, 0.55f, 0.75f};
    Vec3 GroundColor{0.12f, 0.11f, 0.10f};
    f32 IblStrength = 0.25f;
};

} // namespace Muk
