#pragma once

#include "Core/Core.h"
#include <string>
#include <vector>
#include <functional>

namespace Muk {

class AssetManager;
class Renderer;

enum class AssetKind { Unknown, MeshGltf, Texture, Scene, Audio, Script };

struct ContentEntry {
    std::string Path;
    std::string Name;
    AssetKind Kind = AssetKind::Unknown;
    bool Imported = false;
};

class ContentBrowser {
public:
    void SetRoot(const std::string& root);
    const std::string& GetRoot() const { return m_Root; }

    void Rescan();
    const std::vector<ContentEntry>& Entries() const { return m_Entries; }

    // Import glTF / register mesh name; returns true on success
    bool Import(const ContentEntry& entry, AssetManager& assets, Renderer* renderer = nullptr);
    bool Reimport(const ContentEntry& entry, AssetManager& assets, Renderer* renderer = nullptr);

    void DrawImGui(AssetManager& assets, Renderer* renderer,
                   const std::function<void(const ContentEntry&)>& onSelect = nullptr);

private:
    std::string m_Root = "Assets";
    std::vector<ContentEntry> m_Entries;
    char m_PathEdit[260] = "Assets";
    static AssetKind GuessKind(const std::string& path);
};

} // namespace Muk
