#include "ViewportGizmo.h"
#include <cstring>
#include <cmath>

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#include <ImGuizmo.h>
#endif

namespace Muk {

void ViewportGizmo::BeginFrame() {
#ifdef MUK_USE_IMGUI
    ImGuizmo::BeginFrame();
#endif
}

bool ViewportGizmo::Draw(const float* view16, const float* proj16,
                         float viewportX, float viewportY, float viewportW, float viewportH,
                         Transform& transform) {
#ifdef MUK_USE_IMGUI
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(viewportX, viewportY, viewportW, viewportH);

    ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
    if (m_Op == GizmoOp::Rotate) op = ImGuizmo::ROTATE;
    if (m_Op == GizmoOp::Scale) op = ImGuizmo::SCALE;

    float matrix[16];
    Mat4 m = transform.GetMatrix();
    std::memcpy(matrix, m.m, sizeof(matrix));

    // ImGuizmo expects column-major matching our Mat4
    bool changed = ImGuizmo::Manipulate(view16, proj16, op, ImGuizmo::LOCAL, matrix);

    m_Using = ImGuizmo::IsUsing();
    m_Over = ImGuizmo::IsOver();

    if (changed || m_Using) {
        // Decompose matrix into T/R/S (simple extract)
        transform.Position = { matrix[12], matrix[13], matrix[14] };

        // Scale from column lengths
        Vec3 sx = { matrix[0], matrix[1], matrix[2] };
        Vec3 sy = { matrix[4], matrix[5], matrix[6] };
        Vec3 sz = { matrix[8], matrix[9], matrix[10] };
        auto len = [](const Vec3& v) {
            return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
        };
        transform.Scale = { len(sx), len(sy), len(sz) };

        // Euler from rotation (approximate, YXZ)
        if (transform.Scale.x > 1e-5f && transform.Scale.y > 1e-5f && transform.Scale.z > 1e-5f) {
            float r00 = matrix[0] / transform.Scale.x;
            float r10 = matrix[1] / transform.Scale.x;
            float r20 = matrix[2] / transform.Scale.x;
            float r21 = matrix[6] / transform.Scale.y;
            float r22 = matrix[10] / transform.Scale.z;
            transform.Rotation.y = std::atan2(-r20, std::sqrt(r00 * r00 + r10 * r10)) * 57.2957795f;
            transform.Rotation.x = std::atan2(r21, r22) * 57.2957795f;
            transform.Rotation.z = std::atan2(r10, r00) * 57.2957795f;
        }
        return true;
    }
    return false;
#else
    (void)view16; (void)proj16; (void)viewportX; (void)viewportY;
    (void)viewportW; (void)viewportH; (void)transform;
    return false;
#endif
}

} // namespace Muk
