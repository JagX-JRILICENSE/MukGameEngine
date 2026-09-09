#pragma once
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include "ECS/Component.h"
#include <vector>
#include <cstdlib>
#include <cmath>

namespace Muk {

struct VegetationInstance {
    Vec3 Position;
    float Scale = 1.f;
    float Yaw = 0;
};

class VegetationScatter {
public:
    void Scatter(int count, float area, float minScale = 0.3f, float maxScale = 1.2f, unsigned seed = 42) {
        m_Instances.clear();
        std::srand(seed);
        for (int i = 0; i < count; ++i) {
            VegetationInstance inst;
            inst.Position = {
                ((std::rand() % 1000) / 1000.f - 0.5f) * area,
                0,
                ((std::rand() % 1000) / 1000.f - 0.5f) * area
            };
            inst.Scale = minScale + (maxScale - minScale) * ((std::rand() % 1000) / 1000.f);
            inst.Yaw = (float)(std::rand() % 360);
            m_Instances.push_back(inst);
        }
    }

    const std::vector<VegetationInstance>& Instances() const { return m_Instances; }
    int Count() const { return (int)m_Instances.size(); }

    Mat4 InstanceMatrix(const VegetationInstance& i, float windPhase, float windStrength) const {
        float lean = std::sin(windPhase + i.Position.x * 0.3f) * windStrength * 5.f;
        Transform t;
        t.Position = i.Position;
        t.Scale = { i.Scale * 0.15f, i.Scale * 0.8f, i.Scale * 0.15f };
        t.Rotation = { lean, i.Yaw, 0 };
        return t.GetMatrix();
    }

private:
    std::vector<VegetationInstance> m_Instances;
};

struct WindSystem {
    float Strength = 0.4f;
    float Speed = 1.5f;
    Vec3 Direction{ 1, 0, 0.3f };
    float Phase = 0;
    void Update(float dt) { Phase += dt * Speed; }
};

} // namespace Muk
