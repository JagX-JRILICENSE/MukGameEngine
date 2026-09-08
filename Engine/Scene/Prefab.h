#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include "EditorUI/EditorUI.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Muk {

struct PrefabDesc {
    std::string Name;
    std::string MeshName = "Cube";
    std::string MaterialName = "Default";
    Vec3 Scale{1,1,1};
};

class PrefabRegistry {
public:
    void Register(const PrefabDesc& p) { m_Prefabs[p.Name] = p; }
    void RegisterDefaults();

    bool Has(const std::string& name) const { return m_Prefabs.count(name) > 0; }
    const PrefabDesc* Get(const std::string& name) const;

    Entity Spawn(World& world, const std::string& prefabName, const Vec3& position,
                 std::vector<EditorEntityInfo>* track = nullptr);

    const std::unordered_map<std::string, PrefabDesc>& All() const { return m_Prefabs; }

private:
    std::unordered_map<std::string, PrefabDesc> m_Prefabs;
};

} // namespace Muk
