/**
 * Muk Game Engine — REAL Windows application
 * Opens a proper window, clears to dark blue, draws a colored triangle with DX12.
 * This is not a MessageBox stub.
 */
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdint>
#include <string>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using Microsoft::WRL::ComPtr;

static const char* g_Shader = R"(
struct PSInput { float4 pos : SV_POSITION; float4 col : COLOR; };
PSInput VSMain(float3 p : POSITION, float4 c : COLOR) {
    PSInput o; o.pos = float4(p, 1); o.col = c; return o;
}
float4 PSMain(PSInput i) : SV_TARGET { return i.col; }
)";

struct Vertex { float x,y,z; float r,g,b,a; };

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if (m == WM_CLOSE || m == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(h, m, w, l);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hi;
    wc.lpszClassName = L"MukGameEngineWnd";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExW(&wc);

    HWND hwnd = CreateWindowExW(0, L"MukGameEngineWnd", L"Muk Game Engine",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 1280, 720,
        nullptr, nullptr, hi, nullptr);

    ComPtr<IDXGIFactory4> factory;
    CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    ComPtr<ID3D12Device> device;
    D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device));

    D3D12_COMMAND_QUEUE_DESC qd = {};
    qd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;
    device->CreateCommandQueue(&qd, IID_PPV_ARGS(&queue));

    DXGI_SWAP_CHAIN_DESC1 scd = {};
    scd.BufferCount = 2;
    scd.Width = 1280; scd.Height = 720;
    scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.SampleDesc.Count = 1;
    ComPtr<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForHwnd(queue.Get(), hwnd, &scd, nullptr, nullptr, &sc1);
    ComPtr<IDXGISwapChain3> swapchain;
    sc1.As(&swapchain);

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    D3D12_DESCRIPTOR_HEAP_DESC hd = {};
    hd.NumDescriptors = 2;
    hd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&hd, IID_PPV_ARGS(&rtvHeap));
    UINT rtvSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    ComPtr<ID3D12Resource> targets[2];
    D3D12_CPU_DESCRIPTOR_HANDLE rtv = rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < 2; ++i) {
        swapchain->GetBuffer(i, IID_PPV_ARGS(&targets[i]));
        device->CreateRenderTargetView(targets[i].Get(), nullptr, rtv);
        rtv.ptr += rtvSize;
    }

    ComPtr<ID3D12CommandAllocator> alloc;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&alloc));
    ComPtr<ID3D12GraphicsCommandList> cmd;
    device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, alloc.Get(), nullptr, IID_PPV_ARGS(&cmd));
    cmd->Close();

    // Root signature empty
    D3D12_ROOT_SIGNATURE_DESC rsd = {};
    rsd.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> rsBlob;
    D3D12SerializeRootSignature(&rsd, D3D_ROOT_SIGNATURE_VERSION_1, &rsBlob, nullptr);
    ComPtr<ID3D12RootSignature> rootSig;
    device->CreateRootSignature(0, rsBlob->GetBufferPointer(), rsBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig));

    ComPtr<ID3DBlob> vs, ps;
    D3DCompile(g_Shader, strlen(g_Shader), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vs, nullptr);
    D3DCompile(g_Shader, strlen(g_Shader), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &ps, nullptr);

    D3D12_INPUT_ELEMENT_DESC layout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSig.Get();
    psoDesc.VS = { vs->GetBufferPointer(), vs->GetBufferSize() };
    psoDesc.PS = { ps->GetBufferPointer(), ps->GetBufferSize() };
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.InputLayout = { layout, 2 };
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    ComPtr<ID3D12PipelineState> pso;
    device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pso));

    Vertex verts[] = {
        {  0.0f,  0.5f, 0, 1, 0.3f, 0.2f, 1 },
        { -0.5f, -0.5f, 0, 0.2f, 1, 0.3f, 1 },
        {  0.5f, -0.5f, 0, 0.2f, 0.4f, 1, 1 },
    };
    const UINT vbSize = sizeof(verts);
    D3D12_HEAP_PROPERTIES hp = {}; hp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rd = {};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = vbSize; rd.Height = 1; rd.DepthOrArraySize = 1; rd.MipLevels = 1;
    rd.SampleDesc.Count = 1; rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    ComPtr<ID3D12Resource> vb;
    device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&vb));
    void* mapped = nullptr;
    vb->Map(0, nullptr, &mapped);
    memcpy(mapped, verts, vbSize);
    vb->Unmap(0, nullptr);
    D3D12_VERTEX_BUFFER_VIEW vbv = {};
    vbv.BufferLocation = vb->GetGPUVirtualAddress();
    vbv.SizeInBytes = vbSize;
    vbv.StrideInBytes = sizeof(Vertex);

    ComPtr<ID3D12Fence> fence;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    UINT64 fenceValue = 1;

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }

        UINT frame = swapchain->GetCurrentBackBufferIndex();
        alloc->Reset();
        cmd->Reset(alloc.Get(), pso.Get());

        D3D12_RESOURCE_BARRIER bar = {};
        bar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        bar.Transition.pResource = targets[frame].Get();
        bar.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        bar.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        bar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1, &bar);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvHandle.ptr += frame * rtvSize;
        float clear[4] = { 0.08f, 0.10f, 0.16f, 1.0f };
        cmd->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
        cmd->ClearRenderTargetView(rtvHandle, clear, 0, nullptr);

        D3D12_VIEWPORT vp = { 0, 0, 1280, 720, 0, 1 };
        D3D12_RECT scissor = { 0, 0, 1280, 720 };
        cmd->RSSetViewports(1, &vp);
        cmd->RSSetScissorRects(1, &scissor);
        cmd->SetGraphicsRootSignature(rootSig.Get());
        cmd->SetPipelineState(pso.Get());
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0, 1, &vbv);
        cmd->DrawInstanced(3, 1, 0, 0);

        bar.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        bar.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        cmd->ResourceBarrier(1, &bar);
        cmd->Close();

        ID3D12CommandList* lists[] = { cmd.Get() };
        queue->ExecuteCommandLists(1, lists);
        swapchain->Present(1, 0);

        queue->Signal(fence.Get(), fenceValue);
        if (fence->GetCompletedValue() < fenceValue) {
            fence->SetEventOnCompletion(fenceValue, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
        fenceValue++;
    }

    CloseHandle(fenceEvent);
    return 0;
}
