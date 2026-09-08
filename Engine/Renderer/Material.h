#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "Texture.h"
#include <string>
#include <memory>

namespace Muk {

struct Material {
    std::string Name = "Default";

    Vec4 BaseColor{1.0f, 1.0f, 1.0f, 1.0f};
    f32 Metallic = 0.0f;
    f32 Roughness = 0.5f;
    Vec3 Emissive{0.0f, 0.0f, 0.0f};

    std::string AlbedoTexture;
    std::string NormalTexture;
    std::string MetallicRoughnessTexture;
    std::string EmissiveTexture;

    // Runtime texture data (from glTF)
    std::shared_ptr<Texture> AlbedoMap;
    std::shared_ptr<Texture> NormalMap;

    static Material CreateDefault() {
        Material m;
        m.Name = "DefaultLit";
        m.BaseColor = {0.8f, 0.8f, 0.8f, 1.0f};
        return m;
    }

    static Material CreateUnlit(const Vec4& color) {
        Material m;
        m.Name = "Unlit";
        m.BaseColor = color;
        m.Roughness = 1.0f;
        return m;
    }
};

} // namespace Muk
