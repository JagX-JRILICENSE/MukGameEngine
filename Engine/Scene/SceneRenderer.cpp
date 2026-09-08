#include "SceneRenderer.h"
#include "ECS/Component.h"
#include "Core/Profiler.h"

namespace Muk {

void SceneRenderer::Draw(World& world, Renderer& renderer, AssetManager& assets) {
    MUK_PROFILE_SCOPE("SceneRenderer::Draw");

    // Light defaults
    Vec3 lightDir{0.35f, -1.0f, 0.25f};
    Vec3 lightColor{1.0f, 0.98f, 0.92f};
    f32 intensity = 1.2f;
    f32 ambient = 0.18f;

    world.ForEach<DirectionalLight>([&](Entity, DirectionalLight& L) {
        lightDir = L.Direction;
        lightColor = L.Color;
        intensity = L.Intensity;
        ambient = L.Ambient;
    });

    renderer.SetDirectionalLight(lightDir, lightColor, intensity, ambient);

    world.ForEach<Transform, MeshRenderer>([&](Entity, Transform& t, MeshRenderer& mr) {
        if (!mr.Visible) return;
        Material mat = Material::CreateDefault();
        if (auto m = assets.GetMaterial(mr.MaterialName))
            mat = *m;
        renderer.DrawMesh(mr.MeshName, t.GetMatrix(), mat);
    });
}

} // namespace Muk
