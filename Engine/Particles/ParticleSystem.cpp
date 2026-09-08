#include "ParticleSystem.h"
#include "Renderer/Renderer.h"
#include "Renderer/Material.h"
#include <cstdlib>
#include <cmath>

namespace Muk {

static float Rand01() {
    return (float)(std::rand() % 10000) / 10000.0f;
}

void ParticleSystem::Burst(const Vec3& origin, int count, const Vec3& color,
                           float speed, float life, float size) {
    for (int i = 0; i < count; ++i) {
        Particle p;
        p.Position = origin;
        float ax = Rand01() * 6.28318f;
        float ay = Rand01() * 3.14159f;
        p.Velocity = {
            std::cos(ax) * std::sin(ay) * speed * (0.5f + Rand01()),
            std::cos(ay) * speed * (0.5f + Rand01()) + 1.0f,
            std::sin(ax) * std::sin(ay) * speed * (0.5f + Rand01())
        };
        p.Color = color;
        p.Life = life * (0.7f + Rand01() * 0.3f);
        p.MaxLife = p.Life;
        p.Size = size;
        p.Alive = true;
        m_Particles.push_back(p);
    }
}

void ParticleSystem::Update(float dt) {
    for (auto& p : m_Particles) {
        if (!p.Alive) continue;
        p.Life -= dt;
        if (p.Life <= 0) { p.Alive = false; continue; }
        p.Velocity.y -= 4.0f * dt; // gravity
        p.Position.x += p.Velocity.x * dt;
        p.Position.y += p.Velocity.y * dt;
        p.Position.z += p.Velocity.z * dt;
    }
    // compact occasionally
    if (m_Particles.size() > 512) {
        std::vector<Particle> live;
        live.reserve(m_Particles.size() / 2);
        for (auto& p : m_Particles) if (p.Alive) live.push_back(p);
        m_Particles.swap(live);
    }
}

void ParticleSystem::Render(Renderer& renderer) {
    Material mat = Material::CreateDefault();
    for (auto& p : m_Particles) {
        if (!p.Alive) continue;
        float t = p.Life / p.MaxLife;
        float s = p.Size * (0.5f + 0.5f * t);
        Mat4 world = Mat4::Translation(p.Position) * Mat4::Scale({ s, s, s });
        // tint via material albedo if available
        mat.Albedo = { p.Color.x, p.Color.y, p.Color.z, t };
        renderer.DrawMesh("Cube", world, mat);
    }
}

int ParticleSystem::AliveCount() const {
    int n = 0;
    for (auto& p : m_Particles) if (p.Alive) ++n;
    return n;
}

} // namespace Muk
