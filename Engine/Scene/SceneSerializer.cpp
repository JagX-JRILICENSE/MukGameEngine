#include "SceneSerializer.h"
#include "EditorUI/EditorUI.h"
#include "ECS/Component.h"
#include "Core/Log.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Muk {

static std::string Esc(const std::string& s) {
    std::string o; o.reserve(s.size());
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else o += c;
    }
    return o;
}

bool SceneSerializer::SaveWorld(const World& world, const std::string& path,
                                const std::vector<EditorEntityInfo>* names) {
    // world is non-const for ForEach — cast away for read-only iteration pattern
    World& w = const_cast<World&>(world);

    std::ostringstream oss;
    oss << "{\n  \"version\": 1,\n  \"entities\": [\n";

    bool first = true;
    w.ForEach<Transform>([&](Entity e, Transform& t) {
        std::string name = "Entity";
        if (auto* nc = w.GetComponent<NameComponent>(e))
            name = nc->Name;
        else if (names) {
            for (auto& info : *names)
                if (info.Handle.GetID() == e.GetID()) { name = info.Name; break; }
        }

        if (!first) oss << ",\n";
        first = false;
        oss << "    {\n";
        oss << "      \"id\": " << e.GetID() << ",\n";
        oss << "      \"name\": \"" << Esc(name) << "\",\n";
        oss << "      \"transform\": {\n";
        oss << "        \"position\": [" << t.Position.x << "," << t.Position.y << "," << t.Position.z << "],\n";
        oss << "        \"rotation\": [" << t.Rotation.x << "," << t.Rotation.y << "," << t.Rotation.z << "],\n";
        oss << "        \"scale\": [" << t.Scale.x << "," << t.Scale.y << "," << t.Scale.z << "]\n";
        oss << "      }";

        if (auto* mr = w.GetComponent<MeshRenderer>(e)) {
            oss << ",\n      \"mesh_renderer\": {\n";
            oss << "        \"mesh\": \"" << Esc(mr->MeshName) << "\",\n";
            oss << "        \"material\": \"" << Esc(mr->MaterialName) << "\",\n";
            oss << "        \"visible\": " << (mr->Visible ? "true" : "false") << "\n";
            oss << "      }";
        }
        oss << "\n    }";
    });

    oss << "\n  ]\n}\n";

    try {
        std::filesystem::path p(path);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());
    } catch (...) {}

    std::ofstream out(path);
    if (!out) {
        MUK_CORE_ERROR("SaveWorld failed: {0}", path.c_str());
        return false;
    }
    out << oss.str();
    MUK_CORE_INFO("Scene saved: {0}", path.c_str());
    return true;
}

static bool ParseVec3Array(const std::string& s, size_t from, Vec3& out) {
    auto lb = s.find('[', from);
    auto rb = s.find(']', lb);
    if (lb == std::string::npos || rb == std::string::npos) return false;
    float a=0,b=0,c=0;
    if (sscanf(s.c_str() + lb + 1, "%f,%f,%f", &a, &b, &c) != 3) return false;
    out = {a,b,c};
    return true;
}

static std::string ParseStringField(const std::string& block, const char* key) {
    std::string k = std::string("\"") + key + "\"";
    auto p = block.find(k);
    if (p == std::string::npos) return {};
    p = block.find('"', block.find(':', p));
    if (p == std::string::npos) return {};
    size_t start = p + 1;
    size_t end = block.find('"', start);
    if (end == std::string::npos) return {};
    return block.substr(start, end - start);
}

bool SceneSerializer::LoadWorld(World& world, const std::string& path,
                                std::vector<EditorEntityInfo>* outEntities) {
    std::ifstream in(path);
    if (!in) {
        MUK_CORE_ERROR("LoadWorld missing: {0}", path.c_str());
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    std::string json = buffer.str();

    size_t pos = 0;
    int loaded = 0;
    while (true) {
        auto ent = json.find("\"name\"", pos);
        if (ent == std::string::npos) break;
        // Find enclosing object roughly: search back for {
        size_t blockStart = json.rfind('{', ent);
        size_t blockEnd = json.find('}', json.find("transform", ent));
        // Expand to include mesh_renderer if present
        auto mr = json.find("mesh_renderer", ent);
        if (mr != std::string::npos && mr < ent + 800)
            blockEnd = json.find('}', json.find('}', mr) + 1);
        if (blockStart == std::string::npos || blockEnd == std::string::npos) {
            pos = ent + 5;
            continue;
        }
        std::string block = json.substr(blockStart, blockEnd - blockStart + 1);
        pos = blockEnd + 1;

        std::string name = ParseStringField(block, "name");
        if (name.empty()) name = "Loaded";

        Entity e = world.CreateEntity();
        world.AddComponent<NameComponent>(e).Name = name;
        auto& t = world.AddComponent<Transform>(e);

        auto tp = block.find("\"position\"");
        if (tp != std::string::npos) ParseVec3Array(block, tp, t.Position);
        auto tr = block.find("\"rotation\"");
        if (tr != std::string::npos) ParseVec3Array(block, tr, t.Rotation);
        auto ts = block.find("\"scale\"");
        if (ts != std::string::npos) ParseVec3Array(block, ts, t.Scale);

        std::string mesh = ParseStringField(block, "mesh");
        std::string mat = ParseStringField(block, "material");
        if (!mesh.empty() || block.find("mesh_renderer") != std::string::npos) {
            auto& mrC = world.AddComponent<MeshRenderer>(e);
            if (!mesh.empty()) mrC.MeshName = mesh;
            if (!mat.empty()) mrC.MaterialName = mat;
        }

        if (outEntities) {
            EditorEntityInfo info;
            info.Handle = e;
            info.Name = name;
            outEntities->push_back(info);
        }
        ++loaded;
    }

    MUK_CORE_INFO("Scene loaded: {0} entities from {1}", loaded, path.c_str());
    return loaded > 0;
}

} // namespace Muk
