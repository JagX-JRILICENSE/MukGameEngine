#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <string>

namespace Muk {

/**
 * Basic PBR material description.
 * Will later bind to shader parameters and textures.
 */
struct Material {
    std::string Name = "Default";

    // Base color (albedo)
    Vec4 BaseColor{1.0f, 1.0f, 1.0f, 1.0f};

    // Metallic-Roughness workflow
    f32 Metallic = 0.0f;
    f32 Roughness = 0.5f;

    // Optional texture paths (loaded later by Asset system)
    std::string AlbedoTexture;
    std::string NormalTexture;
    std::string MetallicRoughnessTexture;
    std::string EmissiveTexture;

    Vec3 Emissive{0.0f, 0.0f, 0.0f};

    static Material CreateDefault() {
        Material m;
        m.Name = "DefaultLit";
        m.BaseColor = {0.8f, 0.8f, 0.8f, 1.0f};
        m.Metallic = 0.0f;
        m.Roughness = 0.5f;
        return m;
    }

    static Material CreateUnlit(const Vec4& color) {
        Material m;
        m.Name = "Unlit";
        m.BaseColor = color;
        m.Metallic = 0.0f;
        m.Roughness = 1.0f;
        return m;
    }
};

} // namespace Muk
