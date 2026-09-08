# Muk Game Engine v0.5

C++20 / DirectX 12 engine for Windows.

**Not full Unreal** — Phase A foundation with realtime lit+shadowed scene, viewport gizmos, and character controller.

Repo: https://github.com/JagX-JRILICENSE/MukGameEngine · [ROADMAP.md](ROADMAP.md)

---

## Features (v0.5)

| System | Details |
|--------|---------|
| **Shadows** | 1024² directional depth map, comparison sampling, bias |
| **Viewport gizmos** | ImGuizmo translate / rotate / scale (`T` / `R` / `Y`) |
| **Characters** | Jolt `CharacterVirtual` when `-DMUK_USE_JOLT=ON`, else kinematic capsule |
| **Controls** | WASD move, Space jump |
| **Editor** | SceneRT viewport, Hierarchy, Details, Stats, AI BYOK |
| **Lighting** | Directional + ambient + albedo textures |

---

## Build

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_JOLT=ON -DMUK_USE_TINYGLTF=ON
cmake --build build --config Release
```

Or: `./scripts/build_windows.ps1 -Full` then `./scripts/package_windows.ps1`

**CI:** Actions → Windows Build → artifact `MukGameEngine-Windows-x64`

---

## Editor tips

1. Select **Cube** in Hierarchy
2. Press **T** / **R** / **Y** for gizmo mode, drag in Viewport
3. **WASD** + **Space** moves the Player character
4. Paste OpenRouter / NVIDIA / OpenAI key in **AI Control**

---

MIT
