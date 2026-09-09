#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "EditorUI/EditorUI.h"
#include <string>
#include <vector>

namespace Muk {

class Renderer;
class UndoStack;

/**
 * Structured tools the AI agent can invoke (cursor-like control of the editor).
 * Parse ACTION: tool_… lines or call directly from multi-agent pipeline.
 */
class AITools {
public:
    struct Result {
        bool Ok = true;
        std::string Message;
    };

    static Result FocusCamera(Renderer& renderer, const Vec3& target, float distance = 8.0f);
    static Result SelectByName(std::vector<EditorEntityInfo>& entities, Entity& selected,
                               const std::string& name);
    static Result OrbitCamera(Renderer& renderer, float yawDeg, float pitchDeg);
    static Result FrameSelection(Renderer& renderer, World& world, Entity selected);

    /** Parse and run a single ACTION: tool_… line */
    static Result RunLine(const std::string& line, World& world, Renderer& renderer,
                          std::vector<EditorEntityInfo>& entities, Entity& selected);
};

} // namespace Muk
