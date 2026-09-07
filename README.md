# Muk Game Engine

**Muk Game Engine** is a next-generation, high-performance game engine designed to rival Unreal Engine, Unity, Godot, and other industry leaders. Built from the ground up with modern C++20/23, it aims for maximum performance, developer productivity, and visual fidelity on Windows (with cross-platform expansion planned).

> 🚀 **Status**: Early foundation stage. Architecture and core systems are being established. Contributions welcome!

## Vision

Muk aims to deliver:

- **Unreal-level rendering**: Deferred + Forward+, Nanite-inspired virtualized geometry, Lumen-style global illumination, ray tracing support
- **Powerful Editor**: Full visual editor with viewport, outliner, details panel, content browser, Blueprint-like visual scripting
- **ECS-first architecture**: High-performance Entity Component System (EnTT or custom)
- **Physics**: Jolt or Chaos-inspired rigid body, soft body, destruction, vehicles
- **Animation**: Skeletal animation, IK, state machines, blend spaces
- **Audio**: Spatial 3D audio, occlusion, reverb zones
- **Networking**: Client-server, replication, prediction
- **Scripting**: C++ + Lua / AngelScript / Visual Scripting
- **Tools**: Asset pipeline, material editor, particle editor (Niagara-like), sequencer
- **Platform**: Primary target Windows (DirectX 12 / Vulkan), later Linux, macOS, consoles

## Core Architecture (Planned / In Progress)

```
MukGameEngine/
├── Engine/
│   ├── Core/           # Platform abstraction, logging, memory, math, reflection
│   ├── RHI/            # Rendering Hardware Interface (DX12, Vulkan, later Metal)
│   ├── Renderer/       # Deferred renderer, GI, shadows, post-process, Nanite-like
│   ├── ECS/            # Entity Component System
│   ├── Physics/        # Rigid body, collision, constraints
│   ├── Animation/      # Skeletal, IK, animation graphs
│   ├── Audio/          # Spatial audio system
│   ├── Input/          # Action mapping, devices
│   ├── Networking/     # Multiplayer replication
│   ├── Scripting/      # Lua / visual scripting
│   ├── Asset/          # Asset management & pipeline
│   ├── UI/             # Immediate + retained mode UI
│   └── World/          # Scene, levels, streaming
├── Editor/             # Muk Editor (ImGui / custom)
├── Runtime/            # Game runtime executable
├── Plugins/            # Modular plugins
├── ThirdParty/         # Dependencies
├── Samples/            # Example projects
├── Docs/               # Documentation
└── Tools/              # Build tools, asset converters
```

## Features Roadmap

### Phase 1 – Foundation (Current)
- [x] Repository & project structure
- [ ] Platform layer (Windows window, input, file system)
- [ ] Math library (vectors, matrices, quaternions)
- [ ] Logging & assert system
- [ ] Basic RHI (DirectX 12 primary)
- [ ] Simple forward renderer
- [ ] ECS core
- [ ] CMake build system for Windows

### Phase 2 – Core Systems
- [ ] Deferred rendering pipeline
- [ ] PBR materials
- [ ] Shadow mapping
- [ ] Physics integration (Jolt recommended)
- [ ] Skeletal animation
- [ ] Audio (miniaudio / OpenAL)
- [ ] Asset loading (glTF, FBX later)

### Phase 3 – Editor & Tools
- [ ] Muk Editor (viewport, hierarchy, inspector)
- [ ] Content browser
- [ ] Material graph editor
- [ ] Visual scripting (node-based)
- [ ] Sequencer / timeline

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
- Visual Studio 2022 (with C++ Desktop Development workload)
- CMake 3.25+
- Git
- (Optional) Vulkan SDK

### Build Steps
```bash
git clone https://github.com/JagX-JRILICENSE/MukGameEngine.git
cd MukGameEngine
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

The Runtime and Editor executables will appear in `build/bin/`.

## Getting Started (Once Core is Ready)

```cpp
#include <Muk/Engine.h>

int main() {
    Muk::Engine engine;
    engine.Initialize({
        .windowTitle = "Muk Game Engine",
        .width = 1920,
        .height = 1080
    });

    // Create a simple scene
    auto& world = engine.GetWorld();
    auto entity = world.CreateEntity();
    world.AddComponent<Muk::Transform>(entity);
    world.AddComponent<Muk::MeshRenderer>(entity, "Models/Cube.glb");

    engine.Run();
    return 0;
}
```

## License

Muk Game Engine is released under the **MIT License**. You are free to use it for commercial and non-commercial projects.

## Contributing

This is an ambitious open project. Help is welcome in:
- Core systems implementation
- Rendering techniques
- Editor UX
- Documentation
- Samples

Please open issues and pull requests.

## Acknowledgments

Inspired by:
- Unreal Engine (Epic Games)
- Godot Engine
- ezEngine
- Spark Engine
- Bevy / Flecs philosophy
- Modern RHI designs (bgfx, dawn, wgpu)

---

**Muk Game Engine** — Build worlds without limits.
