#include "Prefab.h"
#include "Core/Log.h"

namespace Muk {

void PrefabRegistry::RegisterDefaults() {
    Register({ "Cube", "Cube", "Default", {1,1,1} });
    Register({ "Pillar", "Cube", "Default", {0.4f, 2.0f, 0.4f} });
    Register({ "Orb", "Cube", "Default", {0.35f, 0.35f, 0.35f} });
    Register({ "FloorTile", "Cube", "Default", {2, 0.15f, 2} });
    Register({ "PlayerCapsule", "Cube", "Default", {0.5f, 1.0f, 0.5f} });
}

const PrefabDesc* PrefabRegistry::Get(const std::string& name) const {
    auto it = m_Prefabs.find(name);
    return it == m_Prefabs.end() ? nullptr : &it->second;
}

Entity PrefabRegistry::Spawn(World& world, const std::string& prefabName, const Vec3& position,
                             std::vector<EditorEntityInfo>* track) {
    auto* p = Get(prefabName);
    if (!p) {
        MUK_CORE_WARN("Unknown prefab {0}", prefabName.c_str());
        return Entity();
    }
    auto e = world.CreateEntity();
    std::string name = p->Name;
    world.AddComponent<NameComponent>(e).Name = name;
    auto& t = world.AddComponent<Transform>(e);
    t.Position = position;
    t.Scale = p->Scale;
    auto& mr = world.AddComponent<MeshRenderer>(e);
    mr.MeshName = p->MeshName;
    mr.MaterialName = p->MaterialName;
    if (track) track->push_back({ e, name, false });
    return e;
}

} // namespace Muk
