#pragma once

#include "Core/Core.h"
#include "Renderer/Mesh.h"
#include "Renderer/Material.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace Muk {

/**
 * Central asset manager.
 * Currently supports procedural meshes and material creation.
 * glTF loading interface is ready for tinygltf / cgltf integration.
 */
class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() = default;

    void Initialize();
    void Shutdown();

    // Mesh
    std::shared_ptr<Mesh> GetMesh(const std::string& name);
    std::shared_ptr<Mesh> CreateMesh(const std::string& name, Mesh mesh);
    std::shared_ptr<Mesh> LoadMeshFromGLTF(const std::string& path); // Stub for now

    // Material
    std::shared_ptr<Material> GetMaterial(const std::string& name);
    std::shared_ptr<Material> CreateMaterial(const std::string& name, Material material);

    // Built-in assets
    void RegisterBuiltinAssets();

private:
    std::unordered_map<std::string, std::shared_ptr<Mesh>> m_Meshes;
    std::unordered_map<std::string, std::shared_ptr<Material>> m_Materials;
    bool m_Initialized = false;
};

} // namespace Muk
