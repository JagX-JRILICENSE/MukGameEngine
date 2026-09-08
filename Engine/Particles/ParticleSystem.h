#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"
#include <vector>
#include <string>

namespace Muk {

class Renderer;
struct Material;

struct Particle {
    Vec3 Position{};
    Vec3 Velocity{};
    Vec3 Color{1,1,1};
    float Life = 1.0f;
    float MaxLife = 1.0f;
    float Size = 0.15f;
    bool Alive = true;
};

/** Lightweight CPU particles rendered as scaled cubes via Renderer::DrawMesh */
class ParticleSystem {
public:
    void Burst(const Vec3& origin, int count, const Vec3& color,
               float speed = 3.0f, float life = 0.6f, float size = 0.12f);
    void Update(float dt);
    void Render(Renderer& renderer);

    int AliveCount() const;

private:
    std::vector<Particle> m_Particles;
};

} // namespace Muk
