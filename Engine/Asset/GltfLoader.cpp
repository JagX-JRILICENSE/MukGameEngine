#include "GltfLoader.h"
#include "Core/Log.h"

#include <vector>
#include <cstring>

#ifdef MUK_USE_TINYGLTF
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#endif

namespace Muk {

#ifdef MUK_USE_TINYGLTF
static std::shared_ptr<Texture> ConvertImage(const tinygltf::Image& img, const std::string& name) {
    auto tex = std::make_shared<Texture>();
    tex->Name = name.empty() ? img.name : name;
    tex->Width = (u32)img.width;
    tex->Height = (u32)img.height;
    tex->Channels = 4;

    const size_t pixelCount = (size_t)img.width * (size_t)img.height;
    tex->Pixels.resize(pixelCount * 4);

    if (img.component == 4 && img.bits == 8) {
        std::memcpy(tex->Pixels.data(), img.image.data(), pixelCount * 4);
    } else if (img.component == 3 && img.bits == 8) {
        for (size_t i = 0; i < pixelCount; ++i) {
            tex->Pixels[i * 4 + 0] = img.image[i * 3 + 0];
            tex->Pixels[i * 4 + 1] = img.image[i * 3 + 1];
            tex->Pixels[i * 4 + 2] = img.image[i * 3 + 2];
            tex->Pixels[i * 4 + 3] = 255;
        }
    } else {
        // Fallback white
        for (size_t i = 0; i < pixelCount; ++i) {
            tex->Pixels[i * 4 + 0] = 255;
            tex->Pixels[i * 4 + 1] = 255;
            tex->Pixels[i * 4 + 2] = 255;
            tex->Pixels[i * 4 + 3] = 255;
        }
    }
    return tex;
}
#endif

GltfImportResult GltfLoader::LoadFull(const std::string& path) {
    GltfImportResult result;
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
        return result;
    }

    // Images -> textures
    for (size_t i = 0; i < model.images.size(); ++i) {
        result.Textures.push_back(ConvertImage(model.images[i], model.images[i].name));
    }

    // Material
    auto mat = std::make_shared<Material>(Material::CreateDefault());
    if (!model.materials.empty()) {
        const auto& gm = model.materials[0];
        mat->Name = gm.name.empty() ? "GltfMaterial" : gm.name;
        const auto& pbr = gm.pbrMetallicRoughness;
        if (pbr.baseColorFactor.size() >= 4) {
            mat->BaseColor = {
                (f32)pbr.baseColorFactor[0], (f32)pbr.baseColorFactor[1],
                (f32)pbr.baseColorFactor[2], (f32)pbr.baseColorFactor[3]
            };
        }
        mat->Metallic = (f32)pbr.metallicFactor;
        mat->Roughness = (f32)pbr.roughnessFactor;

        if (pbr.baseColorTexture.index >= 0) {
            int texIndex = pbr.baseColorTexture.index;
            if (texIndex < (int)model.textures.size()) {
                int src = model.textures[texIndex].source;
                if (src >= 0 && src < (int)result.Textures.size()) {
                    mat->AlbedoMap = result.Textures[src];
                    mat->AlbedoTexture = mat->AlbedoMap->Name;
                }
            }
        }
        if (gm.normalTexture.index >= 0) {
            int texIndex = gm.normalTexture.index;
            if (texIndex < (int)model.textures.size()) {
                int src = model.textures[texIndex].source;
                if (src >= 0 && src < (int)result.Textures.size()) {
                    mat->NormalMap = result.Textures[src];
                    mat->NormalTexture = mat->NormalMap->Name;
                }
            }
        }
    }
    result.MaterialData = mat;

    if (model.meshes.empty() || model.meshes[0].primitives.empty()) {
        MUK_CORE_ERROR("glTF has no mesh primitives");
        return result;
    }

    const auto& prim = model.meshes[0].primitives[0];

    auto readAccessor = [&](int accessorIndex, std::vector<float>& outFloats) -> bool {
        if (accessorIndex < 0) return false;
        const auto& acc = model.accessors[accessorIndex];
        const auto& view = model.bufferViews[acc.bufferView];
        const auto& buf = model.buffers[view.buffer];
        const unsigned char* data = buf.data.data() + view.byteOffset + acc.byteOffset;
        int comps = tinygltf::GetNumComponentsInType(acc.type);
        size_t stride = acc.ByteStride(view);
        if (stride == 0)
            stride = tinygltf::GetComponentSizeInBytes(acc.componentType) * comps;

        outFloats.resize(acc.count * comps);
        if (acc.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT) return false;

        for (size_t i = 0; i < acc.count; ++i) {
            const float* src = reinterpret_cast<const float*>(data + i * stride);
            for (int c = 0; c < comps; ++c)
                outFloats[i * comps + c] = src[c];
        }
        return true;
    };

    std::vector<float> positions, normals, texcoords;
    auto posIt = prim.attributes.find("POSITION");
    if (posIt == prim.attributes.end() || !readAccessor(posIt->second, positions))
        return result;

    auto nIt = prim.attributes.find("NORMAL");
    if (nIt != prim.attributes.end()) readAccessor(nIt->second, normals);

    auto tIt = prim.attributes.find("TEXCOORD_0");
    if (tIt != prim.attributes.end()) readAccessor(tIt->second, texcoords);

    size_t vertexCount = positions.size() / 3;
    std::vector<Vertex> vertices(vertexCount);
    for (size_t i = 0; i < vertexCount; ++i) {
        vertices[i].Position = { positions[i*3], positions[i*3+1], positions[i*3+2] };
        vertices[i].Normal = normals.empty() ? Vec3{0,1,0} :
            Vec3{ normals[i*3], normals[i*3+1], normals[i*3+2] };
        vertices[i].TexCoord = texcoords.empty() ? Vec2{0,0} :
            Vec2{ texcoords[i*2], texcoords[i*2+1] };
        // Vertex color from material base color
        vertices[i].Color = mat->BaseColor;
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
            std::memcpy(indices.data(), data, acc.count * sizeof(u32));
        } else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            for (size_t i = 0; i < acc.count; ++i) indices[i] = data[i];
        }
    } else {
        indices.resize(vertexCount);
        for (u32 i = 0; i < (u32)vertexCount; ++i) indices[i] = i;
    }

    Mesh mesh;
    mesh.SetVertices(vertices);
    mesh.SetIndices(indices);
    result.MeshData = std::make_shared<Mesh>(std::move(mesh));
    result.Success = true;

    MUK_CORE_INFO("glTF full import: {0} verts={1} textures={2} mat={3}",
                  path.c_str(), (int)vertexCount, (int)result.Textures.size(), mat->Name.c_str());
#else
    MUK_CORE_WARN("glTF requires -DMUK_USE_TINYGLTF=ON: {0}", path.c_str());
#endif
    return result;
}

std::shared_ptr<Mesh> GltfLoader::Load(const std::string& path) {
    auto full = LoadFull(path);
    return full.MeshData;
}

} // namespace Muk
