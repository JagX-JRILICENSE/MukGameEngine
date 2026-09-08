#pragma once

#include "Core/Core.h"
#include "ECS/World.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetManager.h"

namespace Muk {

/**
 * Draws all MeshRenderer entities using Transform + materials from AssetManager.
 * Applies DirectionalLight from the first entity that has one.
 */
class SceneRenderer {
public:
    void Draw(World& world, Renderer& renderer, AssetManager& assets);
};

} // namespace Muk
