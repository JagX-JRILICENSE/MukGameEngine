#pragma once

#include "Core/Core.h"
#include "ECS/Entity.h"
#include "ECS/World.h"
#include "ECS/Component.h"
#include <string>
#include <vector>
#include <functional>

namespace Muk {

enum class PropType { Float, Float3, Bool, String };

struct PropertyDesc {
    std::string Name;
    PropType Type = PropType::Float;
    // Bound getters/setters for selected entity
    std::function<void(float*)> GetFloat;
    std::function<void(const float*)> SetFloat;
    std::function<void(float*)> GetFloat3; // 3 floats
    std::function<void(const float*)> SetFloat3;
    std::function<bool()> GetBool;
    std::function<void(bool)> SetBool;
    std::function<std::string()> GetString;
    std::function<void(const std::string&)> SetString;
};

struct ComponentView {
    std::string TypeName;
    std::vector<PropertyDesc> Properties;
};

/** Build editable property views for known components on an entity */
class Reflection {
public:
    static std::vector<ComponentView> Inspect(World& world, Entity e);

    // Draw ImGui controls; returns true if any value changed
    static bool DrawImGui(World& world, Entity e);
};

} // namespace Muk
