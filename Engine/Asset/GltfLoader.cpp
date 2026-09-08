#include "GltfLoader.h"
#include "Core/Log.h"

#include <fstream>
#include <vector>
#include <cstring>

#ifdef MUK_USE_TINYGLTF
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#endif

namespace Muk {

std::shared_ptr<Mesh> GltfLoader::Load(const std::string& path) {
#ifdef MUK_USE_TINYGLTF
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool isBinary = path.size() > 4 &&
        (path.substr(path.size() - 4) == ".glb" || path.substr(path.size() - 4) == ".GLB");

    bool ok = isBinary
        ? loader.LoadBinaryFromFile(&model, &err, &warn, path)
        : loader.LoadASCIIFromFile(&model, &err, &warn, path);

    if (!warn.empty()) MUK_CORE_WARN("glTF warn: {0}", warn.c_str());
    if (!ok) {
        MUK_CORE_ERROR("glTF load failed: {0}", err.c_str());
        return nullptr;
    }

    if (model.meshes.empty()) {
        MUK_CORE_ERROR("glTF has no meshes: {0}", path.c_str());
        return nullptr;
    }

    // Load first mesh, first primitive
    const tinygltf::Mesh& gltfMesh = model.meshes[0];
    if (gltfMesh.primitives.empty()) return nullptr;

    const tinygltf::Primitive& prim = gltfMesh.primitives[0];

    auto readAccessor = [&](int accessorIndex, std::vector<float>& outFloats, int& outStride) -> bool {
        if (accessorIndex < 0) return false;
        const auto& acc = model.accessors[accessorIndex];
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf = model.buffers[view.buffer];
        const unsigned char* data = buf.data.data() + view.byteOffset + acc.byteOffset;
        outStride = acc.ByteStride(view) ? (int)acc.ByteStride(view) : (int)(tinygltf::GetComponentSizeInBytes(acc.componentType) * tinygltf::GetNumComponentsInType(acc.type));

        size_t count = acc.count * tinygltf::GetNumComponentsInType(acc.type);
        outFloats.resize(count);

        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT) {
            for (size_t i = 0; i < acc.count; ++i) {
                const float* src = reinterpret_cast<const float*>(data + i * outStride);
                int comps = tinygltf::GetNumComponentsInType(acc.type);
                for (int c = 0; c < comps; ++c)
                    outFloats[i * comps + c] = src[c];
            }
            return true;
        }
        return false;
    };

    std::vector<float> positions, normals, texcoords;
    int stride = 0;

    auto posIt = prim.attributes.find("POSITION");
    if (posIt == prim.attributes.end()) {
        MUK_CORE_ERROR("glTF primitive has no POSITION");
        return nullptr;
    }
    readAccessor(posIt->second, positions, stride);

    auto nIt = prim.attributes.find("NORMAL");
    if (nIt != prim.attributes.end())
        readAccessor(nIt->second, normals, stride);

    auto tIt = prim.attributes.find("TEXCOORD_0");
    if (tIt != prim.attributes.end())
        readAccessor(tIt->second, texcoords, stride);

    size_t vertexCount = positions.size() / 3;
    Mesh mesh;
    std::vector<Vertex> vertices(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        vertices[i].Position = { positions[i * 3 + 0], positions[i * 3 + 1], positions[i * 3 + 2] };
        if (!normals.empty())
            vertices[i].Normal = { normals[i * 3 + 0], normals[i * 3 + 1], normals[i * 3 + 2] };
        else
            vertices[i].Normal = { 0, 1, 0 };
        if (!texcoords.empty())
            vertices[i].TexCoord = { texcoords[i * 2 + 0], texcoords[i * 2 + 1] };
        vertices[i].Color = { 1, 1, 1, 1 };
    }

    std::vector<u32> indices;
    if (prim.indices >= 0) {
        const auto& acc = model.accessors[prim.indices];
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf = model.buffers[view.buffer];
        const unsigned char* data = buf.data.data() + view.byteOffset + acc.byteOffset;
        indices.resize(acc.count);

        if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            for (size_t i = 0; i < acc.count; ++i)
                indices[i] = reinterpret_cast<const uint16_t*>(data)[i];
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            for (size_t i = 0; i < acc.count; ++i)
                indices[i] = reinterpret_cast<const uint32_t*>(data)[i];
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            for (size_t i = 0; i < acc.count; ++i)
                indices[i] = data[i];
        }
    } else {
        indices.resize(vertexCount);
        for (u32 i = 0; i < (u32)vertexCount; ++i) indices[i] = i;
    }

    mesh.SetVertices(vertices);
    mesh.SetIndices(indices);

    MUK_CORE_INFO("glTF loaded: {0} ({1} verts, {2} indices)",
                  path.c_str(), (int)vertexCount, (int)indices.size());
    return std::make_shared<Mesh>(std::move(mesh));

#else
    MUK_CORE_WARN("glTF loading requires MUK_USE_TINYGLTF. Path: {0}", path.c_str());
    MUK_CORE_WARN("Enable with -DMUK_USE_TINYGLTF=ON (FetchContent pulls syoyo/tinygltf)");
    return nullptr;
#endif
}

} // namespace Muk
