# Muk Game Engine

Modern C++20 game engine on Windows / DirectX 12.

> **v0.3** — Depth buffer, real camera MVP, multi-mesh GPU cache, ImGui DX12 font SRV heap, glTF materials/textures, build scripts.

**Repo**: https://github.com/JagX-JRILICENSE/MukGameEngine

---

## What's New in v0.3

### ImGui DX12 font SRV descriptor heap
- Shader-visible `CBV_SRV_UAV` heap (64 slots) on `DX12RHI`
- Slot 0 = ImGui font texture
- `ImGui_ImplDX12_Init` + `RenderDrawData` into the same command list as the scene
- Heap bound every frame in `BeginFrame`

### Depth buffer + real camera MVP
- D32 depth texture + DSV
- Clear depth each frame
- PSO depth test (`LESS`) + depth write
- `Mat4::LookAt` + `Perspective`
- `Renderer::SetCamera` / `CameraView` → `ViewProjection * World` per draw

### Multi-mesh GPU buffer cache
- `DX12Pipeline` caches VB/IB by name (`Triangle`, `Cube`, glTF paths, …)
- `UploadMesh(name, mesh)` once → `DrawMesh(name, world, material)` many times
- Runtime draws **Triangle + Cube** in one frame

### glTF materials & textures
- `GltfLoader::LoadFull` returns mesh + material + images
- `baseColorFactor`, metallic, roughness
- Albedo / normal maps as `Texture` (RGBA8 CPU side)
- `AssetManager::LoadGltfFull` registers everything by path/name

### Build workflow
```bat
scripts\build_windows.bat           REM minimal Release
scripts\build_windows.bat full      REM ImGui + Jolt + tinygltf
scripts\build_windows.bat debug
```
```powershell
./scripts/build_windows.ps1
./scripts/build_windows.ps1 -Full
./scripts/build_windows.ps1 -Config Debug -Full
```

---

## Build (Windows)

**Prerequisites:** VS 2022 (C++ desktop), CMake 3.25+, Git

### One-liner scripts
```bat
git clone https://github.com/JagX-JRILICENSE/MukGameEngine.git
cd MukGameEngine
scripts\build_windows.bat full
```

### Manual CMake
```bash
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DMUK_USE_IMGUI=ON -DMUK_USE_JOLT=ON -DMUK_USE_TINYGLTF=ON
cmake --build . --config Release
```

| Option | Default | Effect |
|--------|---------|--------|
| `MUK_USE_DX12` | ON | DirectX 12 RHI |
| `MUK_USE_IMGUI` | OFF | Dear ImGui docking + DX12 fonts |
| `MUK_USE_JOLT` | OFF | Jolt Physics |
| `MUK_USE_TINYGLTF` | OFF | glTF mesh/material/texture load |

**Outputs:** `build/bin/Release/MukRuntime.exe`, `MukEditor.exe`

Expect a dark viewport with **perspective triangle + cube** (depth tested). With ImGui ON, docking panels draw on top.

---

## Quick API

```cpp
#include "Engine.h"
using namespace Muk;

class MyGame : public Application {
  void OnInit() override {
    CameraView cam;
    cam.Eye = {0, 1.5f, -4};
    cam.Target = {0, 0, 0};
    Renderer().SetCamera(cam);

    Renderer().UploadMesh("Triangle", *Assets().GetMesh("Triangle"));
    Renderer().UploadMesh("Cube", *Assets().GetMesh("Cube"));

    // Full glTF (needs MUK_USE_TINYGLTF):
    // auto r = Assets().LoadGltfFull("Assets/model.glb");
    // if (r.Success) Renderer().UploadMesh("model", *r.MeshData);
  }

  void OnRender() override {
    auto mat = Assets().GetMaterial("Default");
    Renderer().DrawMesh("Triangle", Mat4::Translation({-1,0,0}), *mat);
    Renderer().DrawMesh("Cube", Mat4::Translation({1,0,0}), *mat);
  }
};
```

---

## Architecture (v0.3)

```
Engine/
  RHI/DX12/   DX12RHI (RTV, DSV, ImGui SRV heap) + DX12Pipeline (mesh cache, depth PSO)
  Renderer/   CameraView, Mesh, Material, Texture, Shaders
  Asset/      AssetManager, GltfLoader (mesh + PBR material + images)
  EditorUI/   ImGui docking + DX12 RenderDrawData
  Physics/    Simple + Jolt
  Math/       LookAt, Perspective, Mat4 mul
scripts/      build_windows.bat / .ps1
```

---

## Roadmap

- [x] DX12 triangle / PSO / buffers
- [x] ImGui DX12 font SRV heap + GPU UI draw
- [x] Depth buffer + camera MVP
- [x] Multi-mesh GPU cache
- [x] glTF materials / textures (CPU)
- [ ] Upload glTF albedo to GPU SRV + sample in HLSL
- [ ] Render-to-texture editor viewport
- [ ] Audio, animation, networking

---

MIT License — **Muk Game Engine**
