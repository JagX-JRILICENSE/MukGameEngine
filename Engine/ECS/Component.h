#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Muk {

// Base marker for components
struct IComponent {
    virtual ~IComponent() = default;
};

struct Transform : public IComponent {
    Vec3 Position{0.0f, 0.0f, 0.0f};
    Vec3 Rotation{0.0f, 0.0f, 0.0f}; // Euler for now
    Vec3 Scale{1.0f, 1.0f, 1.0f};

    Mat4 GetMatrix() const {
        // Simple composition: T * R * S (rotation not fully implemented yet)
        return Mat4::Translation(Position) * Mat4::Scale(Scale);
    }
};

struct MeshRenderer : public IComponent {
    std::string MeshPath;
    // Material handle, etc. later
};

struct Camera : public IComponent {
    f32 FOV = 60.0f;
    f32 Near = 0.1f;
    f32 Far = 1000.0f;
    bool Primary = true;
};

} // namespace Muk
