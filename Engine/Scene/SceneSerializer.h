#pragma once

#include "Core/Core.h"
#include "ECS/World.h"
#include <string>
#include <vector>

namespace Muk {

struct EditorEntityInfo;

class SceneSerializer {
public:
    // Save world entities with Name, Transform, MeshRenderer to JSON
    static bool SaveWorld(const World& world, const std::string& path,
                          const std::vector<EditorEntityInfo>* names = nullptr);

    // Clear-and-load not applied: returns list of entities created into world
    static bool LoadWorld(World& world, const std::string& path,
                          std::vector<EditorEntityInfo>* outEntities = nullptr);
};

} // namespace Muk
