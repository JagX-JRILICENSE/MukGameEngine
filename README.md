# Muk Game Engine v0.6

C++20 / DirectX 12 Windows engine.

**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine · [ROADMAP.md](ROADMAP.md)

---

## Current features

### Rendering
- DX12 device, swapchain, depth buffer
- Mesh cache, albedo GPU textures, lit Lambert + metal/rough terms
- **Cascaded shadows (3)** Texture2DArray + **soft 3×3 PCF**
- Editor SceneRT viewport (render-to-texture)

### Editor
- Docking panels: Hierarchy, Details, Viewport, Console, Content, Stats, AI, Toolbar
- **ImGuizmo** T/R/Y in viewport
- **Undo / Redo** (Ctrl+Z / Ctrl+Y) for transforms
- **Play-In-Editor** (F5 / Toolbar) — snapshot scene, play, stop restores

### Gameplay / physics
- ECS: Transform, MeshRenderer, DirectionalLight, Camera, RigidBody, Name
- Jolt Physics (optional) + simple fallback
- **CharacterVirtual** (or kinematic) — WASD + Space

### Assets / AI
- glTF path (tinygltf), materials/textures
- **BYOK AI**: OpenRouter, NVIDIA, OpenAI, custom

### Build / ship
- CMake + `scripts/build_windows.ps1 -Full`
- GitHub Actions Windows artifact

---

## Controls

| Input | Action |
|-------|--------|
| T / R / Y | Gizmo mode |
| Ctrl+Z / Ctrl+Y | Undo / Redo |
| F5 | Play / Stop PIE |
| WASD + Space | Character (especially in Play) |

---

## Build

```powershell
./scripts/build_windows.ps1 -Full
```

`-DMUK_USE_IMGUI=ON -DMUK_USE_JOLT=ON -DMUK_USE_TINYGLTF=ON`

---

## What to add next (priority)

1. **Per-pixel cascade selection** (upload all 3 LightVPs + splits to CB)
2. **Animation** (skinned meshes / clips)
3. **Audio** (spatial)
4. **Content Browser** import + reimport
5. **Reflection / property UI** for all components
6. **Save / load** world JSON
7. **PBR IBL** + post (bloom, tonemap)
8. **Deeper AI actions** (materials, spawn prefabs)

MIT
