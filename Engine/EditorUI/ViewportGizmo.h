#pragma once

#include "Core/Core.h"
#include "Math/Matrix.h"
#include "Math/Vector.h"
#include "ECS/Component.h"

namespace Muk {

enum class GizmoOp {
    Translate,
    Rotate,
    Scale
};

/**
 * ImGuizmo wrapper for viewport manipulation of Transform components.
 * Call Draw inside the Viewport ImGui window after Image().
 */
class ViewportGizmo {
public:
    void BeginFrame();
    // Returns true if matrix was edited
    bool Draw(const float* view16, const float* proj16,
              float viewportX, float viewportY, float viewportW, float viewportH,
              Transform& transform);

    void SetOperation(GizmoOp op) { m_Op = op; }
    GizmoOp GetOperation() const { return m_Op; }
    bool IsUsing() const { return m_Using; }
    bool IsOver() const { return m_Over; }

private:
    GizmoOp m_Op = GizmoOp::Translate;
    bool m_Using = false;
    bool m_Over = false;
};

} // namespace Muk
