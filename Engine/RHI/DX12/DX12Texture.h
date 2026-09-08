#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <string>
#include <unordered_map>
#include <memory>
#include "Core/Core.h"
#include "Renderer/Texture.h"

namespace Muk {

using Microsoft::WRL::ComPtr;

struct GPUTexture {
    ComPtr<ID3D12Resource> Resource;
    D3D12_CPU_DESCRIPTOR_HANDLE CpuSrv = {};
    D3D12_GPU_DESCRIPTOR_HANDLE GpuSrv = {};
    u32 Width = 0;
    u32 Height = 0;
    bool Valid = false;
};

/**
 * Uploads CPU Texture (RGBA8) to GPU DEFAULT heap and creates SRV
 * on the shared shader-visible heap provided by DX12RHI.
 */
class DX12TextureCache {
public:
    void Initialize(ID3D12Device* device, ID3D12DescriptorHeap* srvHeap,
                    u32 srvDescriptorSize, u32* srvBumpIndex, u32 srvMaxCount);
    void Shutdown();

    // Returns GPU handle; creates GPU texture if missing
    bool Upload(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList,
                ID3D12CommandQueue* queue, const std::string& name,
                const Texture& cpuTex);

    bool Has(const std::string& name) const;
    GPUTexture* Get(const std::string& name);

    // 1x1 white fallback always available after Init
    D3D12_GPU_DESCRIPTOR_HANDLE GetWhiteSrv() const { return m_WhiteGpu; }

private:
    bool CreateWhiteTexture(ID3D12Device* device);

    ID3D12Device* m_Device = nullptr;
    ID3D12DescriptorHeap* m_SrvHeap = nullptr;
    u32 m_SrvSize = 0;
    u32* m_SrvNext = nullptr;
    u32 m_SrvMax = 0;

    std::unordered_map<std::string, std::unique_ptr<GPUTexture>> m_Cache;
    ComPtr<ID3D12Resource> m_WhiteResource;
    D3D12_GPU_DESCRIPTOR_HANDLE m_WhiteGpu = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_WhiteCpu = {};
};

} // namespace Muk
