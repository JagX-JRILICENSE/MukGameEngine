#pragma once
#include "Core/Core.h"
#include "Math/Vector.h"
#include "PostSettings.h"

namespace Muk {

/** Extended post stack beyond basic bloom/IBL — settings drive future full-screen passes */
struct PostStack {
    PostSettings Base;

    // SSAO
    bool SSAO = true;
    float SSAORadius = 0.5f;
    float SSAOIntensity = 0.85f;
    int SSAOSamples = 16;

    // Depth of field
    bool DOF = false;
    float FocusDistance = 8.0f;
    float Aperture = 0.15f;
    float FocalLength = 50.0f;

    // Motion blur
    bool MotionBlur = false;
    float ShutterAngle = 180.f;
    int MotionSamples = 8;

    // Color grading
    float Contrast = 1.05f;
    float Saturation = 1.1f;
    float Temperature = 0.0f; // -1 cool .. +1 warm
    Vec3 Lift{0,0,0};
    Vec3 Gamma{1,1,1};
    Vec3 Gain{1,1,1};

    // Fog tie-in
    bool VolumetricFog = false;
    float VolDensity = 0.02f;

    // Tone map curve
    enum class ToneMap { ACES, Reinhard, Uncharted2 } Tone = ToneMap::ACES;

    /** Apply color grade to a linear RGB sample (CPU preview / shader mirror) */
    Vec3 Grade(Vec3 c) const {
        c.x = (c.x + Lift.x) * Gain.x;
        c.y = (c.y + Lift.y) * Gain.y;
        c.z = (c.z + Lift.z) * Gain.z;
        c.x = std::pow(std::max(c.x, 0.f), 1.f / std::max(Gamma.x, 0.01f));
        c.y = std::pow(std::max(c.y, 0.f), 1.f / std::max(Gamma.y, 0.01f));
        c.z = std::pow(std::max(c.z, 0.f), 1.f / std::max(Gamma.z, 0.01f));
        float luma = 0.2126f*c.x + 0.7152f*c.y + 0.0722f*c.z;
        c.x = luma + (c.x - luma) * Saturation;
        c.y = luma + (c.y - luma) * Saturation;
        c.z = luma + (c.z - luma) * Saturation;
        c.x = (c.x - 0.5f) * Contrast + 0.5f;
        c.y = (c.y - 0.5f) * Contrast + 0.5f;
        c.z = (c.z - 0.5f) * Contrast + 0.5f;
        c.x += Temperature * 0.05f;
        c.z -= Temperature * 0.05f;
        return c;
    }
};

} // namespace Muk
