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
    void RegisterMesh(const std::string& name, std::shared_ptr<Mesh> mesh) {
        if (mesh) m_Meshes[name] = std::move(mesh);
    }

    GltfImportResult LoadGltfFull(const std::string& path);

    std::shared_ptr<Material> GetMaterial(const std::string& name);
    std::shared_ptr<Material> CreateMaterial(const std::string& name, Material material);
    void RegisterMaterial(const std::string& name, std::shared_ptr<Material> mat) {
        if (mat) m_Materials[name] = std::move(mat);
    }

    std::shared_ptr<Texture> GetTexture(const std::string& name);
    void RegisterTexture(const std::string& name, std::shared_ptr<Texture> tex) {
        if (tex) m_Textures[name] = std::move(tex);
    }

    void RegisterBuiltinAssets();

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
    std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
    bool m_Initialized = false;
};

} // namespace Muk
