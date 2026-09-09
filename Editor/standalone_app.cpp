/**
 * Muk Game Engine v0.16 — REAL Windows DirectX 12 application
 * NOT a MessageBox stub. 1280x720 window, lit multi-cube scene, orbit camera, FPS title.
 */
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <vector>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using Microsoft::WRL::ComPtr;

static const wchar_t* kWndClass = L"MukGameEngineMainWnd";
static const int kW = 1280;
static const int kH = 720;

struct V3 { float x, y, z; };
struct M4 {
    float m[16];
    static M4 Id() { M4 r{}; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1; return r; }
    static M4 Perspective(float fovY, float aspect, float n, float f) {
        M4 r{}; float t = tanf(fovY * 0.5f);
        r.m[0]=1.f/(aspect*t); r.m[5]=1.f/t; r.m[10]=f/(n-f); r.m[11]=-1.f; r.m[14]=(n*f)/(n-f); return r;
    }
    static M4 LookAt(V3 eye, V3 tgt, V3 up) {
        auto sub=[](V3 a,V3 b){return V3{a.x-b.x,a.y-b.y,a.z-b.z};};
        auto cross=[](V3 a,V3 b){return V3{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};};
        auto norm=[](V3 v){ float l=sqrtf(v.x*v.x+v.y*v.y+v.z*v.z); return l>1e-6f?V3{v.x/l,v.y/l,v.z/l}:V3{};};
        auto dot=[](V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;};
        V3 z=norm(sub(eye,tgt)); V3 x=norm(cross(up,z)); V3 y=cross(z,x);
        M4 r=Id();
        r.m[0]=x.x;r.m[4]=x.y;r.m[8]=x.z;r.m[12]=-dot(x,eye);
        r.m[1]=y.x;r.m[5]=y.y;r.m[9]=y.z;r.m[13]=-dot(y,eye);
        r.m[2]=z.x;r.m[6]=z.y;r.m[10]=z.z;r.m[14]=-dot(z,eye);
        return r;
    }
    M4 operator*(const M4& o) const {
        M4 r{};
        for(int c=0;c<4;++c) for(int row=0;row<4;++row)
            r.m[c*4+row]=m[0*4+row]*o.m[c*4+0]+m[1*4+row]*o.m[c*4+1]+m[2*4+row]*o.m[c*4+2]+m[3*4+row]*o.m[c*4+3];
        return r;
    }
};

struct Vertex { float px,py,pz, nx,ny,nz, r,g,b,a; };
struct FrameCB { float mvp[16]; float world[16]; float lightDir[4]; float lightCol[4]; float camPos[4]; };

static const char* g_HLSL = R"(
cbuffer FrameCB : register(b0) {
    float4x4 MVP; float4x4 World; float4 LightDir; float4 LightCol; float4 CamPos;
};
struct VSIn { float3 P:POSITION; float3 N:NORMAL; float4 C:COLOR; };
struct VSOut { float4 Pos:SV_POSITION; float3 N:NORMAL; float4 C:COLOR; float3 W:TEXCOORD0; };
VSOut VSMain(VSIn v) {
    VSOut o; float4 wp=mul(float4(v.P,1),World); o.W=wp.xyz;
    o.Pos=mul(float4(v.P,1),MVP); o.N=normalize(mul(float4(v.N,0),World).xyz); o.C=v.C; return o;
}
float4 PSMain(VSOut i) : SV_TARGET {
    float3 N=normalize(i.N); float3 L=normalize(-LightDir.xyz);
    float ndl=saturate(dot(N,L));
    float3 col=i.C.rgb*(LightCol.xyz*0.18 + LightCol.xyz*LightDir.w*ndl);
    float3 V=normalize(CamPos.xyz-i.W);
    col += pow(1.0-saturate(dot(N,V)),2.0)*0.15;
    return float4(col,1);
}
)";

static void AppendCube(std::vector<Vertex>& v, std::vector<uint32_t>& idx,
    float cx,float cy,float cz,float sx,float sy,float sz,float r,float g,float b) {
    struct F{float px,py,pz,nx,ny,nz;};
    const F faces[]={
        {-1,-1,1,0,0,1},{1,-1,1,0,0,1},{1,1,1,0,0,1},{-1,1,1,0,0,1},
        {1,-1,-1,0,0,-1},{-1,-1,-1,0,0,-1},{-1,1,-1,0,0,-1},{1,1,-1,0,0,-1},
        {1,-1,1,1,0,0},{1,-1,-1,1,0,0},{1,1,-1,1,0,0},{1,1,1,1,0,0},
        {-1,-1,-1,-1,0,0},{-1,-1,1,-1,0,0},{-1,1,1,-1,0,0},{-1,1,-1,-1,0,0},
        {-1,1,1,0,1,0},{1,1,1,0,1,0},{1,1,-1,0,1,0},{-1,1,-1,0,1,0},
        {-1,-1,-1,0,-1,0},{1,-1,-1,0,-1,0},{1,-1,1,0,-1,0},{-1,-1,1,0,-1,0},
    };
    uint32_t base=(uint32_t)v.size();
    for(auto& f:faces){
        Vertex vt{}; vt.px=cx+f.px*sx*0.5f; vt.py=cy+f.py*sy*0.5f; vt.pz=cz+f.pz*sz*0.5f;
        vt.nx=f.nx;vt.ny=f.ny;vt.nz=f.nz; vt.r=r;vt.g=g;vt.b=b;vt.a=1; v.push_back(vt);
    }
    for(int face=0;face<6;++face){
        uint32_t i0=base+face*4;
        idx.push_back(i0);idx.push_back(i0+1);idx.push_back(i0+2);
        idx.push_back(i0);idx.push_back(i0+2);idx.push_back(i0+3);
    }
}

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    if(m==WM_CLOSE||m==WM_DESTROY){PostQuitMessage(0);return 0;}
    if(m==WM_KEYDOWN&&w==VK_ESCAPE){PostQuitMessage(0);return 0;}
    return DefWindowProcW(h,m,w,l);
}

static void Fail(HWND hwnd, const char* msg) {
    MessageBoxA(hwnd, msg, "Muk Game Engine — error", MB_OK|MB_ICONERROR);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int) {
    SetProcessDPIAware();
    WNDCLASSEXW wc={sizeof(wc)};
    wc.style=CS_HREDRAW|CS_VREDRAW; wc.lpfnWndProc=WndProc; wc.hInstance=hi;
    wc.hCursor=LoadCursor(nullptr,IDC_ARROW); wc.hbrBackground=(HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName=kWndClass; wc.hIcon=LoadIcon(nullptr,IDI_APPLICATION);
    RegisterClassExW(&wc);

    RECT rc={0,0,kW,kH}; AdjustWindowRect(&rc,WS_OVERLAPPEDWINDOW,FALSE);
    HWND hwnd=CreateWindowExW(0,kWndClass,L"Muk Game Engine  |  DirectX 12 Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT,CW_USEDEFAULT,
        rc.right-rc.left, rc.bottom-rc.top, nullptr,nullptr,hi,nullptr);
    if(!hwnd) return 1;

    ComPtr<IDXGIFactory4> factory;
    if(FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))){Fail(hwnd,"DXGI factory failed");return 1;}
    ComPtr<ID3D12Device> device;
    if(FAILED(D3D12CreateDevice(nullptr,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){
        Fail(hwnd,"No DirectX 12 GPU. Need a DX12 Windows PC.");return 1;
    }
    D3D12_COMMAND_QUEUE_DESC qd={}; qd.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue; device->CreateCommandQueue(&qd,IID_PPV_ARGS(&queue));

    DXGI_SWAP_CHAIN_DESC1 scd={};
    scd.BufferCount=2; scd.Width=kW; scd.Height=kH; scd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; scd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
    scd.SampleDesc.Count=1;
    ComPtr<IDXGISwapChain1> sc1;
    factory->CreateSwapChainForHwnd(queue.Get(),hwnd,&scd,nullptr,nullptr,&sc1);
    ComPtr<IDXGISwapChain3> swapchain; sc1.As(&swapchain);
    factory->MakeWindowAssociation(hwnd,DXGI_MWA_NO_ALT_ENTER);

    ComPtr<ID3D12DescriptorHeap> rtvHeap;
    D3D12_DESCRIPTOR_HEAP_DESC hd={}; hd.NumDescriptors=2; hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&rtvHeap));
    UINT rtvSize=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    ComPtr<ID3D12DescriptorHeap> dsvHeap;
    D3D12_DESCRIPTOR_HEAP_DESC dhd={}; dhd.NumDescriptors=1; dhd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    device->CreateDescriptorHeap(&dhd,IID_PPV_ARGS(&dsvHeap));
    D3D12_RESOURCE_DESC depthDesc={};
    depthDesc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthDesc.Width=kW; depthDesc.Height=kH; depthDesc.DepthOrArraySize=1; depthDesc.MipLevels=1;
    depthDesc.Format=DXGI_FORMAT_D32_FLOAT; depthDesc.SampleDesc.Count=1;
    depthDesc.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    D3D12_CLEAR_VALUE depthClear={}; depthClear.Format=DXGI_FORMAT_D32_FLOAT; depthClear.DepthStencil.Depth=1.f;
    D3D12_HEAP_PROPERTIES defHeap={}; defHeap.Type=D3D12_HEAP_TYPE_DEFAULT;
    ComPtr<ID3D12Resource> depthBuf;
    device->CreateCommittedResource(&defHeap,D3D12_HEAP_FLAG_NONE,&depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,&depthClear,IID_PPV_ARGS(&depthBuf));
    device->CreateDepthStencilView(depthBuf.Get(),nullptr,dsvHeap->GetCPUDescriptorHandleForHeapStart());

    ComPtr<ID3D12Resource> targets[2];
    D3D12_CPU_DESCRIPTOR_HANDLE rtv=rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for(UINT i=0;i<2;++i){ swapchain->GetBuffer(i,IID_PPV_ARGS(&targets[i]));
        device->CreateRenderTargetView(targets[i].Get(),nullptr,rtv); rtv.ptr+=rtvSize; }

    ComPtr<ID3D12CommandAllocator> alloc;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&alloc));
    ComPtr<ID3D12GraphicsCommandList> cmd;
    device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,alloc.Get(),nullptr,IID_PPV_ARGS(&cmd));
    cmd->Close();

    D3D12_ROOT_PARAMETER param={};
    param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_CBV; param.Descriptor.ShaderRegister=0;
    param.ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
    D3D12_ROOT_SIGNATURE_DESC rsd={}; rsd.NumParameters=1; rsd.pParameters=&param;
    rsd.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    ComPtr<ID3DBlob> rsBlob,errBlob;
    D3D12SerializeRootSignature(&rsd,D3D_ROOT_SIGNATURE_VERSION_1,&rsBlob,&errBlob);
    ComPtr<ID3D12RootSignature> rootSig;
    device->CreateRootSignature(0,rsBlob->GetBufferPointer(),rsBlob->GetBufferSize(),IID_PPV_ARGS(&rootSig));

    ComPtr<ID3DBlob> vs,ps;
    if(FAILED(D3DCompile(g_HLSL,strlen(g_HLSL),"muk",nullptr,nullptr,"VSMain","vs_5_0",0,0,&vs,&errBlob))||
       FAILED(D3DCompile(g_HLSL,strlen(g_HLSL),"muk",nullptr,nullptr,"PSMain","ps_5_0",0,0,&ps,&errBlob))){
        Fail(hwnd,"Shader compile failed"); return 1;
    }

    D3D12_INPUT_ELEMENT_DESC layout[]={
        {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
        {"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
    };
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc={};
    psoDesc.pRootSignature=rootSig.Get();
    psoDesc.VS={vs->GetBufferPointer(),vs->GetBufferSize()};
    psoDesc.PS={ps->GetBufferPointer(),ps->GetBufferSize()};
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
    psoDesc.SampleMask=UINT_MAX;
    psoDesc.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode=D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.DepthClipEnable=TRUE;
    psoDesc.DepthStencilState.DepthEnable=TRUE;
    psoDesc.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;
    psoDesc.DSVFormat=DXGI_FORMAT_D32_FLOAT;
    psoDesc.InputLayout={layout,3};
    psoDesc.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets=1; psoDesc.RTVFormats[0]=DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count=1;
    ComPtr<ID3D12PipelineState> pso;
    if(FAILED(device->CreateGraphicsPipelineState(&psoDesc,IID_PPV_ARGS(&pso)))){Fail(hwnd,"PSO failed");return 1;}

    std::vector<Vertex> verts; std::vector<uint32_t> indices;
    AppendCube(verts,indices,0,-0.05f,0,14,0.1f,14,0.28f,0.30f,0.34f);
    AppendCube(verts,indices,0,0.6f,0,1.2f,1.2f,1.2f,0.95f,0.55f,0.20f);
    AppendCube(verts,indices,2.4f,0.5f,1.2f,1,1,1,0.25f,0.65f,0.95f);
    AppendCube(verts,indices,-2.2f,0.5f,-1.0f,1,1,1,0.35f,0.85f,0.40f);
    AppendCube(verts,indices,1.5f,1.4f,-2.0f,0.8f,0.8f,0.8f,0.90f,0.30f,0.55f);
    AppendCube(verts,indices,-1.2f,0.35f,2.0f,0.7f,0.7f,0.7f,0.95f,0.90f,0.25f);

    auto upload=[&](const void* data,size_t size,ComPtr<ID3D12Resource>& out){
        D3D12_HEAP_PROPERTIES hp={}; hp.Type=D3D12_HEAP_TYPE_UPLOAD;
        D3D12_RESOURCE_DESC rd={}; rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;
        rd.Width=size; rd.Height=1; rd.DepthOrArraySize=1; rd.MipLevels=1;
        rd.SampleDesc.Count=1; rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&out));
        void* m=nullptr; out->Map(0,nullptr,&m); memcpy(m,data,size); out->Unmap(0,nullptr);
    };
    ComPtr<ID3D12Resource> vb,ib,cbuf;
    upload(verts.data(),verts.size()*sizeof(Vertex),vb);
    upload(indices.data(),indices.size()*sizeof(uint32_t),ib);
    std::vector<uint8_t> cbPad(256,0); upload(cbPad.data(),cbPad.size(),cbuf);
    void* cbMapped=nullptr; cbuf->Map(0,nullptr,&cbMapped);

    D3D12_VERTEX_BUFFER_VIEW vbv={}; vbv.BufferLocation=vb->GetGPUVirtualAddress();
    vbv.SizeInBytes=(UINT)(verts.size()*sizeof(Vertex)); vbv.StrideInBytes=sizeof(Vertex);
    D3D12_INDEX_BUFFER_VIEW ibv={}; ibv.BufferLocation=ib->GetGPUVirtualAddress();
    ibv.SizeInBytes=(UINT)(indices.size()*sizeof(uint32_t)); ibv.Format=DXGI_FORMAT_R32_UINT;

    ComPtr<ID3D12Fence> fence; device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
    HANDLE fenceEvent=CreateEvent(nullptr,FALSE,FALSE,nullptr); UINT64 fenceValue=1;

    LARGE_INTEGER freq,t0,t1; QueryPerformanceFrequency(&freq); QueryPerformanceCounter(&t0);
    float time=0; int frames=0; float fpsTimer=0; int fps=0;

    MSG msg={};
    while(msg.message!=WM_QUIT){
        while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){
            TranslateMessage(&msg); DispatchMessageW(&msg);
            if(msg.message==WM_QUIT) break;
        }
        if(msg.message==WM_QUIT) break;

        QueryPerformanceCounter(&t1);
        float dt=float(t1.QuadPart-t0.QuadPart)/float(freq.QuadPart); t0=t1;
        time+=dt; fpsTimer+=dt; frames++;
        if(fpsTimer>=0.5f){
            fps=(int)(frames/fpsTimer); frames=0; fpsTimer=0;
            wchar_t title[160];
            swprintf_s(title,L"Muk Game Engine  |  DX12 Demo  |  %d FPS  |  ESC quit",fps);
            SetWindowTextW(hwnd,title);
        }

        float radius=9.f, angle=time*0.35f;
        V3 eye{sinf(angle)*radius,4.2f,cosf(angle)*radius}; V3 target{0,0.6f,0};
        M4 view=M4::LookAt(eye,target,V3{0,1,0});
        M4 proj=M4::Perspective(60.f*3.14159265f/180.f,float(kW)/float(kH),0.1f,200.f);
        M4 world=M4::Id(); M4 mvp=proj*view*world;
        FrameCB cb{}; memcpy(cb.mvp,mvp.m,64); memcpy(cb.world,world.m,64);
        cb.lightDir[0]=0.4f;cb.lightDir[1]=-1.f;cb.lightDir[2]=0.3f;cb.lightDir[3]=1.35f;
        cb.lightCol[0]=1;cb.lightCol[1]=0.97f;cb.lightCol[2]=0.92f;cb.lightCol[3]=1;
        cb.camPos[0]=eye.x;cb.camPos[1]=eye.y;cb.camPos[2]=eye.z;cb.camPos[3]=1;
        memcpy(cbMapped,&cb,sizeof(cb));

        UINT frameIdx=swapchain->GetCurrentBackBufferIndex();
        alloc->Reset(); cmd->Reset(alloc.Get(),pso.Get());
        D3D12_RESOURCE_BARRIER bar={}; bar.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        bar.Transition.pResource=targets[frameIdx].Get();
        bar.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;
        bar.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;
        bar.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        cmd->ResourceBarrier(1,&bar);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvH=rtvHeap->GetCPUDescriptorHandleForHeapStart();
        rtvH.ptr+=frameIdx*rtvSize;
        D3D12_CPU_DESCRIPTOR_HANDLE dsv=dsvHeap->GetCPUDescriptorHandleForHeapStart();
        cmd->OMSetRenderTargets(1,&rtvH,FALSE,&dsv);
        float clear[4]={0.07f,0.09f,0.14f,1.f};
        cmd->ClearRenderTargetView(rtvH,clear,0,nullptr);
        cmd->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.f,0,0,nullptr);

        D3D12_VIEWPORT vp={0,0,(float)kW,(float)kH,0,1};
        D3D12_RECT scissor={0,0,kW,kH};
        cmd->RSSetViewports(1,&vp); cmd->RSSetScissorRects(1,&scissor);
        cmd->SetGraphicsRootSignature(rootSig.Get()); cmd->SetPipelineState(pso.Get());
        cmd->SetGraphicsRootConstantBufferView(0,cbuf->GetGPUVirtualAddress());
        cmd->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        cmd->IASetVertexBuffers(0,1,&vbv); cmd->IASetIndexBuffer(&ibv);
        cmd->DrawIndexedInstanced((UINT)indices.size(),1,0,0,0);

        bar.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;
        bar.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;
        cmd->ResourceBarrier(1,&bar); cmd->Close();
        ID3D12CommandList* lists[]={cmd.Get()}; queue->ExecuteCommandLists(1,lists);
        swapchain->Present(1,0);
        queue->Signal(fence.Get(),fenceValue);
        if(fence->GetCompletedValue()<fenceValue){
            fence->SetEventOnCompletion(fenceValue,fenceEvent);
            WaitForSingleObject(fenceEvent,INFINITE);
        }
        ++fenceValue;
    }
    CloseHandle(fenceEvent);
    return 0;
}
