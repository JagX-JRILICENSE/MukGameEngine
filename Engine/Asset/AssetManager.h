#pragma once

#include "Core/Core.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include "Renderer/Texture.h"
#include "GltfLoader.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Muk {

class AssetManager {
public:
    void Initialize();
    void Shutdown();

    std::shared_ptr<Mesh> GetMesh(const std::string& name);
    std::shared_ptr<Mesh> CreateMesh(const std::string& name, Mesh mesh);
    std::shared_ptr<Mesh> LoadMeshFromGLTF(const std::string& path);

    // Full import: mesh + material + textures registered
    GltfImportResult LoadGltfFull(const std::string& path);

    std::shared_ptr<Material> GetMaterial(const std::string& name);
    std::shared_ptr<Material> CreateMaterial(const std::string& name, Material material);

    std::shared_ptr<Texture> GetTexture(const std::string& name);

    void RegisterBuiltinAssets();

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    bool m_Initialized = false;
};

} // namespace Muk
