# Muk Game Engine

**Muk Game Engine** is a next-generation, high-performance game engine designed to rival Unreal Engine, Unity, Godot, and other industry leaders. Built from the ground up with modern C++20, it aims for maximum performance, developer productivity, and visual fidelity on Windows (with cross-platform expansion planned).

> 🚀 **Status**: Foundation + Core Systems in active development. DX12 path is live.

## Vision

Muk aims to deliver:

- **Unreal-level rendering**: Deferred + Forward+, Nanite-inspired virtualized geometry, Lumen-style global illumination, ray tracing support
- **Powerful Editor**: Full visual editor with viewport, outliner, details panel, content browser, Blueprint-like visual scripting
- **ECS-first architecture**: High-performance Entity Component System
- **Physics**: Jolt-ready rigid body system (simple solver now, full Jolt later)
- **Animation**: Skeletal animation, IK, state machines, blend spaces
- **Audio**: Spatial 3D audio, occlusion, reverb zones
- **Networking**: Client-server, replication, prediction
- **Scripting**: C++ + Lua / AngelScript / Visual Scripting
- **Tools**: Asset pipeline, material editor, particle editor, sequencer
- **Platform**: Primary target Windows (DirectX 12), later Linux, macOS, consoles

## What Works Right Now

### Step 1 – Real DirectX 12 RHI
- Device creation (hardware + WARP fallback)
- Command queue, allocators, graphics command list
- Flip-model swapchain
- RTV heap + double buffering
- Frame synchronization with fences
- Clear color + viewport/scissor
- Resize support

### Step 2 – Forward Renderer + Meshes
- `Mesh` class with Vertex format (Position, Normal, TexCoord, Color)
- Built-in primitives: **Triangle**, **Cube**, **Quad**
- `Renderer::DrawMesh()` submission API (GPU path next)
- Dark clear color so you can see the window is alive

### Step 3 – Physics
- `PhysicsWorld` with create/destroy body API
- Body types: Static / Dynamic / Kinematic
- Shapes: Box, Sphere, Capsule
- Gravity + simple Euler integration + ground plane collision
- Designed so **Jolt Physics** can replace the solver with minimal API changes
- `RigidBodyComponent` for ECS linking

### Step 4 – Editor Foundation
- `MukEditor` executable
- Hierarchy seed entities (Camera, Light, Floor, Player Start)
- Viewport / Details / Content Browser layout planned (ImGui next)
- Physics test body spawned on editor start

### Step 5 – Assets & Materials
- `AssetManager` with builtin meshes & materials
- PBR-ready `Material` (BaseColor, Metallic, Roughness, Emissive + texture slots)
- `LoadMeshFromGLTF()` interface ready for tinygltf / cgltf

## Architecture

```
MukGameEngine/
├── Engine/
│   ├── Core/           Application, Window (Win32), Log
│   ├── Math/           Vec2/3/4, Mat4
│   ├── RHI/            Abstraction + DX12 backend
│   ├── Renderer/       Mesh, Material, Renderer
│   ├── ECS/            Entity, World, Components
│   ├── Physics/        PhysicsWorld (Jolt-ready)
│   ├── Asset/          AssetManager
│   └── Input/
├── Editor/             Muk Editor
├── Runtime/            Game / Sandbox executable
├── Samples/
└── CMakeLists.txt
```

## Features Roadmap

### Phase 1 – Foundation ✅
- [x] Repository & project structure
- [x] Platform layer (Windows window)
- [x] Math library
- [x] Logging system
- [x] DirectX 12 RHI (device, swapchain, frames)
- [x] Simple forward renderer structure
- [x] ECS core
- [x] CMake build system for Windows

### Phase 2 – Core Systems (In Progress)
- [x] Mesh primitives + Material system
- [x] Physics abstraction + simple solver
- [x] AssetManager + glTF interface
- [ ] Full triangle draw with vertex/index buffers + shaders
- [ ] PBR shading
- [ ] Shadow mapping
- [ ] Real Jolt Physics integration
- [ ] Skeletal animation
- [ ] Audio

### Phase 3 – Editor & Tools
- [x] Editor host + entity hierarchy seed
- [ ] Dear ImGui docking (Viewport, Hierarchy, Details)
- [ ] Content browser
- [ ] Material graph editor
- [ ] Visual scripting
- [ ] Sequencer

### Phase 4 – Advanced
- [ ] Virtualized geometry (Nanite-inspired)
- [ ] Dynamic GI (Lumen-inspired)
- [ ] Hardware ray tracing
- [ ] Full multiplayer
- [ ] Advanced particles
- [ ] World partitioning / streaming

## Building on Windows

### Prerequisites
- Windows 10/11
- Visual Studio 2022 (C++ Desktop Development workload)
- CMake 3.25+
- Git

### Build Steps
```bash
git clone https://github.com/JagX-JRILICENSE/MukGameEngine.git
cd MukGameEngine
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

Executables appear in `build/bin/`:
- `MukRuntime.exe` – Sandbox / game host
- `MukEditor.exe` – Editor host

You should see a dark blue-gray window (DX12 clear color). Physics bodies are simulated in the background.

## Quick Example

```cpp
#include "Engine.h"

using namespace Muk;

class MyGame : public Application {
protected:
    void OnInit() override {
        auto entity = ECS().CreateEntity();
        auto& t = ECS().AddComponent<Transform>(entity);
        t.Position = {0, 2, 0};

        RigidBodyDesc desc;
        desc.Type = BodyType::Dynamic;
        desc.Shape = ShapeType::Box;
        desc.Position = {0, 5, 0};
        Physics().CreateBody(desc);
    }

    void OnRender() override {
        auto mesh = Assets().GetMesh("Cube");
        auto mat  = Assets().GetMaterial("Default");
        if (mesh && mat)
            Renderer().DrawMesh(*mesh, Mat4::Identity(), *mat);
    }
};

int main() {
    MyGame app;
    app.Run();
}
```

## License

MIT License — free for commercial and non-commercial use.

## Contributing

Help is welcome on:
- Completing the DX12 triangle (root signature, PSO, vertex buffers)
- ImGui editor panels
- Jolt Physics integration
- glTF loading (tinygltf)
- Documentation & samples

Open issues and pull requests!

---

**Muk Game Engine** — Build worlds without limits.
