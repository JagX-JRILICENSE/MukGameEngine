#include "AssetManager.h"
#include "GltfLoader.h"
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

    MUK_CORE_INFO("Registered builtin meshes and materials");
}

std::shared_ptr<Mesh> AssetManager::GetMesh(const std::string& name) {
    auto it = m_Meshes.find(name);
    if (it != m_Meshes.end()) return it->second;
    MUK_CORE_WARN("Mesh not found: {0}", name.c_str());
    return nullptr;
}

std::shared_ptr<Mesh> AssetManager::CreateMesh(const std::string& name, Mesh mesh) {
    auto ptr = std::make_shared<Mesh>(std::move(mesh));
    m_Meshes[name] = ptr;
    return ptr;
}

std::shared_ptr<Mesh> AssetManager::LoadMeshFromGLTF(const std::string& path) {
    auto mesh = GltfLoader::Load(path);
    if (!mesh) {
        MUK_CORE_WARN("glTF failed, returning builtin Cube for: {0}", path.c_str());
        return GetMesh("Cube");
    }
    // Cache under path key
    m_Meshes[path] = mesh;
    return mesh;
}

std::shared_ptr<Material> AssetManager::GetMaterial(const std::string& name) {
    auto it = m_Materials.find(name);
    if (it != m_Materials.end()) return it->second;
    MUK_CORE_WARN("Material not found: {0}", name.c_str());
    return nullptr;
}

std::shared_ptr<Material> AssetManager::CreateMaterial(const std::string& name, Material material) {
    auto ptr = std::make_shared<Material>(std::move(material));
    m_Materials[name] = ptr;
    return ptr;
}

} // namespace Muk
