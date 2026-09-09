#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <string>
#include <cmath>

namespace Muk {

struct IComponent {
    virtual ~IComponent() = default;
};

struct Transform : public IComponent {
    Vec3 Position{0.0f, 0.0f, 0.0f};
    Vec3 Rotation{0.0f, 0.0f, 0.0f};
    Vec3 Scale{1.0f, 1.0f, 1.0f};

    Mat4 GetMatrix() const {
        const f32 deg2rad = 0.01745329251f;
        f32 rx = Rotation.x * deg2rad;
        f32 ry = Rotation.y * deg2rad;
        f32 rz = Rotation.z * deg2rad;

        Mat4 S = Mat4::Scale(Scale);

        Mat4 Rx = Mat4::Identity();
        Rx.m[5] = std::cos(rx); Rx.m[6] = -std::sin(rx);
        Rx.m[9] = std::sin(rx); Rx.m[10] = std::cos(rx);

        Mat4 Ry = Mat4::Identity();
        Ry.m[0] = std::cos(ry); Ry.m[2] = std::sin(ry);
        Ry.m[8] = -std::sin(ry); Ry.m[10] = std::cos(ry);

        Mat4 Rz = Mat4::Identity();
        Rz.m[0] = std::cos(rz); Rz.m[1] = -std::sin(rz);
        Rz.m[4] = std::sin(rz); Rz.m[5] = std::cos(rz);

        return Mat4::Translation(Position) * Rz * Ry * Rx * S;
    }
};

struct MeshRenderer : public IComponent {
    std::string MeshName = "Cube";
    std::string MaterialName = "Default";
    bool Visible = true;
    int LODBias = 0;
    float BoundingRadius = 1.0f;
};

struct Camera : public IComponent {
    f32 FOV = 60.0f;
    f32 Near = 0.1f;
    f32 Far = 1000.0f;
    bool Primary = true;
};

struct DirectionalLight : public IComponent {
    Vec3 Direction{0.3f, -1.0f, 0.2f};
    Vec3 Color{1.0f, 0.98f, 0.95f};
    f32 Intensity = 1.2f;
    f32 Ambient = 0.15f;
};

struct RigidBodyComponent : public IComponent {
    EntityID BodyId = 0;
    bool SyncTransform = true;
};

struct NameComponent : public IComponent {
    std::string Name = "Entity";
};

struct TagComponent : public IComponent {
    std::string Tag = "Untagged";
    int Layer = 0; // 0=Default, 1=UI, 2=IgnoreRaycast, 3=Water, 4=AI
};

struct BillboardComponent : public IComponent {
    bool FaceCamera = true;
    bool AxisY = true;
};

} // namespace Muk
