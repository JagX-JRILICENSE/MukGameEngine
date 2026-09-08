#pragma once

#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/Texture.h"
#include <string>
#include <memory>
#include <vector>

namespace Muk {

struct GltfImportResult {
    std::shared_ptr<Mesh> MeshData;
    std::shared_ptr<Material> MaterialData;
    std::vector<std::shared_ptr<Texture>> Textures;
    bool Success = false;
};

class GltfLoader {
public:
    static std::shared_ptr<Mesh> Load(const std::string& path);
    static GltfImportResult LoadFull(const std::string& path);
};

} // namespace Muk
