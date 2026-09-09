#include "Screenshot.h"
#include "Renderer/Renderer.h"
#include "RHI/DX12/DX12RHI.h"
#include "Core/Log.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>
#include <algorithm>

#ifdef MUK_PLATFORM_WINDOWS
#include <d3d12.h>
#include <wrl/client.h>
#endif

namespace Muk {

bool Screenshot::WriteTgaRGBA(const std::string& path, u32 w, u32 h, const u8* rgba) {
    std::ofstream out(path, std::ios::binary);
    if (!out || !rgba) return false;
    unsigned char header[18] = {};
    header[2] = 2;
    header[12] = (unsigned char)(w & 255);
    header[13] = (unsigned char)((w >> 8) & 255);
    header[14] = (unsigned char)(h & 255);
    header[15] = (unsigned char)((h >> 8) & 255);
    header[16] = 24;
    out.write(reinterpret_cast<char*>(header), 18);
    std::vector<unsigned char> row(w * 3);
    for (int y = (int)h - 1; y >= 0; --y) {
        for (u32 x = 0; x < w; ++x) {
            const u8* p = rgba + ((u32)y * w + x) * 4;
            row[x * 3 + 0] = p[2];
            row[x * 3 + 1] = p[1];
            row[x * 3 + 2] = p[0];
        }
        out.write(reinterpret_cast<char*>(row.data()), row.size());
    }
    return true;
}

bool Screenshot::WriteSolidTga(const std::string& path, u32 w, u32 h, u8 r, u8 g, u8 b) {
    std::vector<u8> rgba(w * h * 4);
    for (u32 i = 0; i < w * h; ++i) {
        rgba[i * 4 + 0] = r; rgba[i * 4 + 1] = g; rgba[i * 4 + 2] = b; rgba[i * 4 + 3] = 255;
    }
    return WriteTgaRGBA(path, w, h, rgba.data());
}

static std::string MakeShotPath(const std::string& label) {
    std::filesystem::create_directories("Assets/Screenshots");
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream name;
    name << "Assets/Screenshots/shot_" << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_" << label << ".tga";
    return name.str();
}

bool Screenshot::CapturePlaceholder(const std::string& label) {
    try {
        std::string path = MakeShotPath(label);
        if (!WriteSolidTga(path, 320, 180, 40, 30, 70)) return false;
        MUK_CORE_INFO("Screenshot: {0}", path.c_str());
        return true;
    } catch (...) { return false; }
}

bool Screenshot::CaptureSceneRT(Renderer& renderer, const std::string& label) {
#ifdef MUK_PLATFORM_WINDOWS
    auto* dx = dynamic_cast<DX12RHI*>(renderer.GetRHI());
    u32 w = renderer.GetSceneRTWidth();
    u32 h = renderer.GetSceneRTHeight();
    ID3D12Resource* color = renderer.GetSceneRTColorResource();
    if (!dx || !color || w == 0 || h == 0)
        return CapturePlaceholder(label);

    D3D12_RESOURCE_DESC desc = color->GetDesc();
    u64 rowPitch = ((u64)w * 4 + 255) & ~255ull;
    u64 total = rowPitch * h;

    D3D12_HEAP_PROPERTIES hp = {};
    hp.Type = D3D12_HEAP_TYPE_READBACK;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = total;
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    Microsoft::WRL::ComPtr<ID3D12Resource> staging;
    if (FAILED(dx->GetDevice()->CreateCommittedResource(
            &hp, D3D12_HEAP_FLAG_NONE, &rd,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&staging))))
        return CapturePlaceholder(label);

    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> alloc;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> list;
    dx->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
    dx->GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&list));

    D3D12_TEXTURE_COPY_LOCATION srcLoc = {};
    srcLoc.pResource = color;
    srcLoc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLoc.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLoc = {};
    dstLoc.pResource = staging.Get();
    dstLoc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dx->GetDevice()->GetCopyableFootprints(&desc, 0, 1, 0, &dstLoc.PlacedFootprint, nullptr, nullptr, nullptr);

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = color;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    list->ResourceBarrier(1, &barrier);
    list->CopyTextureRegion(&dstLoc, 0, 0, 0, &srcLoc, nullptr);
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_SOURCE;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    list->ResourceBarrier(1, &barrier);
    list->Close();

    ID3D12CommandList* lists[] = { list.Get() };
    dx->GetCommandQueue()->ExecuteCommandLists(1, lists);
    dx->WaitForGPUPublic();

    std::vector<u8> pixels(w * h * 4);
    void* mapped = nullptr;
    if (SUCCEEDED(staging->Map(0, nullptr, &mapped))) {
        u8* srcp = (u8*)mapped;
        for (u32 y = 0; y < h; ++y)
            memcpy(pixels.data() + y * w * 4, srcp + y * rowPitch, w * 4);
        staging->Unmap(0, nullptr);
    } else {
        return CapturePlaceholder(label);
    }

    std::string path = MakeShotPath(label);
    if (!WriteTgaRGBA(path, w, h, pixels.data())) return false;
    MUK_CORE_INFO("GPU screenshot {0}x{1}: {2}", w, h, path.c_str());
    return true;
#else
    (void)renderer;
    return CapturePlaceholder(label);
#endif
}

} // namespace Muk
