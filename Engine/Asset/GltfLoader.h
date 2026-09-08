#pragma once

#include "Renderer/Mesh.h"
#include <string>
#include <memory>

namespace Muk {

/**
 * Loads mesh data from glTF 2.0 (.gltf / .glb).
 * Uses tinygltf when MUK_USE_TINYGLTF is defined.
 * Otherwise attempts a minimal built-in path for simple assets.
 */
class GltfLoader {
public:
    static std::shared_ptr<Mesh> Load(const std::string& path);
};

} // namespace Muk
