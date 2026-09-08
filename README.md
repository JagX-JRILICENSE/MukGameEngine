# Muk Game Engine

**Muk Game Engine** — modern C++20 game engine targeting Unreal-class features, starting on Windows / DirectX 12.

> **Status**: v0.2 — DX12 triangle path complete, Editor docking UI, Jolt-ready physics, glTF loader.

**Repo**: https://github.com/JagX-JRILICENSE/MukGameEngine

---

## What's New in v0.2

### 1. Complete DX12 Triangle Path
- Root signature (CBV for MVP)
- Graphics PSO + input layout (POSITION, NORMAL, TEXCOORD, COLOR)
- HLSL vertex + pixel shaders (embedded + `Engine/Renderer/Shaders/Basic.hlsl`)
- Runtime compile via `D3DCompile`
- Vertex / index buffer upload
- `Renderer::UploadMesh` + `DrawMesh` → real `DrawIndexedInstanced`

### 2. Dear ImGui Docking Editor Panels
- `EditorUI` with dockspace host
- Panels: **Hierarchy**, **Details**, **Viewport**, **Content Browser**, **Console**
- Menu bar (File / Edit / Window)
- Transform inspector (drag floats) when ImGui is enabled
- Console fallback when built without ImGui

Enable:
```bash
cmake .. -DMUK_USE_IMGUI=ON
```

### 3. Real Jolt Physics Integration Path
- `PhysicsWorld` dual backend:
  - **Default**: simple CPU solver (always works)
  - **Jolt**: full body create/destroy, gravity, step, position/velocity queries
- Box / Sphere / Capsule shapes
- Static / Dynamic / Kinematic

Enable:
```bash
cmake .. -DMUK_USE_JOLT=ON
```

### 4. Actual glTF Loading
- `GltfLoader` via **tinygltf** (`.gltf` + `.glb`)
- Reads POSITION, NORMAL, TEXCOORD_0 + index buffer
- Cached in `AssetManager`

Enable:
```bash
cmake .. -DMUK_USE_TINYGLTF=ON
```

---

## Build (Windows)

### Minimal (DX12 triangle, simple physics)
```bash
git clone https://github.com/JagX-JRILICENSE/MukGameEngine.git
cd MukGameEngine
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Full feature set
```bash
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DMUK_USE_IMGUI=ON ^
  -DMUK_USE_JOLT=ON ^
  -DMUK_USE_TINYGLTF=ON
cmake --build . --config Release
```

First full configure will fetch ImGui (docking branch), Jolt, and tinygltf via CMake `FetchContent`.

Executables: `build/bin/MukRuntime.exe`, `build/bin/MukEditor.exe`

You should see a **colored triangle** on a dark blue-gray clear.

---

## Architecture

```
Engine/
  Core/           Application, Window (Win32), Log
  Math/           Vec2/3/4, Mat4
  RHI/DX12/       DX12RHI + DX12Pipeline (root sig, PSO, buffers)
  Renderer/       Mesh, Material, Renderer, Shaders/Basic.hlsl
  ECS/            Entity, World, Components
  Physics/        PhysicsWorld (simple + Jolt)
  Asset/          AssetManager, GltfLoader
  EditorUI/       Dear ImGui docking panels
  Input/
Editor/           Muk Editor host
Runtime/          Sandbox / game host
```

---

## Quick API

```cpp
#include "Engine.h"
using namespace Muk;

class MyGame : public Application {
  void OnInit() override {
    auto mesh = Assets().GetMesh("Triangle");
    Renderer().UploadMesh(*mesh);

    // Or load glTF (needs MUK_USE_TINYGLTF):
    // auto model = Assets().LoadMeshFromGLTF("Assets/model.glb");

    RigidBodyDesc desc;
    desc.Type = BodyType::Dynamic;
    desc.Shape = ShapeType::Box;
    desc.Position = {0, 5, 0};
    Physics().CreateBody(desc);
  }

  void OnRender() override {
    auto mesh = Assets().GetMesh("Triangle");
    auto mat  = Assets().GetMaterial("Default");
    Renderer().DrawMesh(*mesh, Mat4::Scale({0.8f,0.8f,0.8f}), *mat);
  }
};
```

---

## Roadmap

- [x] DX12 device, swapchain, triangle draw
- [x] Editor docking panel structure
- [x] Jolt integration path
- [x] glTF mesh load path
- [ ] Full ImGui DX12 descriptor heap wiring (SRV heap for fonts)
- [ ] Depth buffer + camera MVP
- [ ] Multiple mesh GPU buffers
- [ ] Jolt layer interface polish + character controller
- [ ] Materials / textures from glTF
- [ ] Audio, animation, networking

---

## License

MIT — free for commercial and non-commercial use.

**Muk Game Engine** — Build worlds without limits.
