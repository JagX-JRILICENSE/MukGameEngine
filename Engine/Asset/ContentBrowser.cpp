#include "ContentBrowser.h"
#include "AssetManager.h"
#include "GltfLoader.h"
#include "Renderer/Renderer.h"
#include "Core/Log.h"
#include <filesystem>
#include <cstring>

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

namespace fs = std::filesystem;

AssetKind ContentBrowser::GuessKind(const std::string& path) {
    auto ext = fs::path(path).extension().string();
    for (auto& c : ext) c = (char)tolower((unsigned char)c);
    if (ext == ".gltf" || ext == ".glb") return AssetKind::MeshGltf;
    if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga") return AssetKind::Texture;
    if (ext == ".json" || ext == ".mukscene") return AssetKind::Scene;
    if (ext == ".wav" || ext == ".ogg") return AssetKind::Audio;
    if (ext == ".muk" || ext == ".txt") return AssetKind::Script;
    return AssetKind::Unknown;
}

void ContentBrowser::SetRoot(const std::string& root) {
    m_Root = root;
    std::snprintf(m_PathEdit, sizeof(m_PathEdit), "%s", root.c_str());
}

void ContentBrowser::Rescan() {
    m_Entries.clear();
    std::error_code ec;
    fs::create_directories(m_Root + "/Scripts", ec);
    fs::create_directories(m_Root + "/Scenes", ec);
    fs::create_directories(m_Root + "/Screenshots", ec);
    if (!fs::exists(m_Root, ec)) {
        fs::create_directories(m_Root, ec);
        return;
    }
    for (auto it = fs::recursive_directory_iterator(m_Root, ec);
         it != fs::recursive_directory_iterator(); ++it) {
        if (!it->is_regular_file()) continue;
        ContentEntry e;
        e.Path = it->path().string();
        e.Name = it->path().filename().string();
        e.Kind = GuessKind(e.Path);
        if (e.Kind != AssetKind::Unknown)
            m_Entries.push_back(std::move(e));
    }
    MUK_CORE_INFO("ContentBrowser scan: {0} assets", (int)m_Entries.size());
}

bool ContentBrowser::Import(const ContentEntry& entry, AssetManager& assets, Renderer* renderer) {
    if (entry.Kind == AssetKind::MeshGltf) {
        auto full = GltfLoader::LoadFull(entry.Path);
        if (!full.Success || !full.MeshData) return false;
        std::string key = fs::path(entry.Name).stem().string();
        assets.RegisterMesh(key, full.MeshData);
        if (full.MaterialData)
            assets.RegisterMaterial(key + "_Mat", full.MaterialData);
        if (renderer) {
            renderer->UploadMesh(key, *full.MeshData);
            for (auto& tex : full.Textures)
                if (tex) renderer->UploadTexture(tex->Name, *tex);
        }
        MUK_CORE_INFO("Imported glTF as mesh '{0}'", key.c_str());
        return true;
    }
    return false;
}

bool ContentBrowser::Reimport(const ContentEntry& entry, AssetManager& assets, Renderer* renderer) {
    return Import(entry, assets, renderer);
}

void ContentBrowser::DrawImGui(AssetManager& assets, Renderer* renderer,
                               const std::function<void(const ContentEntry&)>& onSelect) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Content Browser");
    if (ImGui::InputText("Root", m_PathEdit, sizeof(m_PathEdit),
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        SetRoot(m_PathEdit);
        Rescan();
    }
    if (ImGui::Button("Rescan")) Rescan();
    ImGui::SameLine();
    if (ImGui::Button("Open Assets")) { SetRoot("Assets"); Rescan(); }
    ImGui::Separator();
    ImGui::BeginChild("assetlist");
    for (auto& e : m_Entries) {
        const char* kind = "?";
        if (e.Kind == AssetKind::MeshGltf) kind = "glTF";
        else if (e.Kind == AssetKind::Texture) kind = "Tex";
        else if (e.Kind == AssetKind::Scene) kind = "Scene";
        else if (e.Kind == AssetKind::Audio) kind = "Audio";
        else if (e.Kind == AssetKind::Script) kind = "Script";

        ImGui::PushID(e.Path.c_str());
        if (ImGui::Selectable(e.Name.c_str(), false)) {
            if (onSelect) onSelect(e);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("[%s]", kind);
        if (e.Kind == AssetKind::MeshGltf) {
            ImGui::SameLine();
            if (ImGui::SmallButton("Import")) Import(e, assets, renderer);
            ImGui::SameLine();
            if (ImGui::SmallButton("Reimport")) Reimport(e, assets, renderer);
        }
        ImGui::PopID();
    }
    if (m_Entries.empty())
        ImGui::TextDisabled("AI builds save .muk scripts here after Rescan");
    ImGui::EndChild();
    ImGui::End();
#else
    (void)assets; (void)renderer; (void)onSelect;
#endif
}

} // namespace Muk
