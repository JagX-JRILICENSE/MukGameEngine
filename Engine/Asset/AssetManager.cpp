#include "AssetManager.h"
#include "Core/Log.h"

namespace Muk {

void AssetManager::Initialize() {
    RegisterBuiltinAssets();
    m_Initialized = true;
    MUK_CORE_INFO("AssetManager initialized");
}

void AssetManager::Shutdown() {
    m_Meshes.clear();
    m_Materials.clear();
    m_Textures.clear();
    m_Initialized = false;
}

void AssetManager::RegisterBuiltinAssets() {
    CreateMesh("Triangle", Mesh::CreateTriangle());
    CreateMesh("Cube", Mesh::CreateCube());
    CreateMesh("Quad", Mesh::CreateQuad());

    CreateMaterial("Default", Material::CreateDefault());
    CreateMaterial("Red", Material::CreateUnlit({1, 0, 0, 1}));
    CreateMaterial("Green", Material::CreateUnlit({0, 1, 0, 1}));
    CreateMaterial("Blue", Material::CreateUnlit({0, 0, 1, 1}));

    auto white = Texture::CreateSolid(4, 4, 255, 255, 255);
    white->Name = "White";
    m_Textures["White"] = white;
}

std::shared_ptr<Mesh> AssetManager::GetMesh(const std::string& name) {
    auto it = m_Meshes.find(name);
    return it != m_Meshes.end() ? it->second : nullptr;
}

std::shared_ptr<Mesh> AssetManager::CreateMesh(const std::string& name, Mesh mesh) {
    auto ptr = std::make_shared<Mesh>(std::move(mesh));
    m_Meshes[name] = ptr;
    return ptr;
}

std::shared_ptr<Mesh> AssetManager::LoadMeshFromGLTF(const std::string& path) {
    auto full = LoadGltfFull(path);
    return full.MeshData ? full.MeshData : GetMesh("Cube");
}

GltfImportResult AssetManager::LoadGltfFull(const std::string& path) {
    auto result = GltfLoader::LoadFull(path);
    if (!result.Success) {
        MUK_CORE_WARN("LoadGltfFull failed: {0}", path.c_str());
        return result;
    }

    m_Meshes[path] = result.MeshData;

    if (result.MaterialData) {
        std::string matName = result.MaterialData->Name.empty() ? path + "_mat" : result.MaterialData->Name;
        m_Materials[matName] = result.MaterialData;
        m_Materials[path + "_mat"] = result.MaterialData;
    }

    for (size_t i = 0; i < result.Textures.size(); ++i) {
        auto& tex = result.Textures[i];
        std::string key = tex->Name.empty() ? path + "_tex" + std::to_string(i) : tex->Name;
        m_Textures[key] = tex;
        if (result.MaterialData && result.MaterialData->AlbedoMap == tex)
            result.MaterialData->AlbedoTexture = key;
    }

    MUK_CORE_INFO("Registered glTF assets from {0}", path.c_str());
    return result;
}

std::shared_ptr<Material> AssetManager::GetMaterial(const std::string& name) {
    auto it = m_Materials.find(name);
    return it != m_Materials.end() ? it->second : nullptr;
}

std::shared_ptr<Material> AssetManager::CreateMaterial(const std::string& name, Material material) {
    auto ptr = std::make_shared<Material>(std::move(material));
    m_Materials[name] = ptr;
    return ptr;
}

std::shared_ptr<Texture> AssetManager::GetTexture(const std::string& name) {
    auto it = m_Textures.find(name);
    return it != m_Textures.end() ? it->second : nullptr;
}

} // namespace Muk
