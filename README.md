# Muk Game Engine v0.12

https://github.com/JagX-JRILICENSE/MukGameEngine

## Roadmap next-5 (done)

1. **GPU skinning path** — `Skin.hlsl` + `SkinCBData` bone buffer (`SkinGPU.h`)
2. **Bloom extract** — bright-pass helper + live bloom strength in lighting
3. **Cubemap IBL** — procedural 6-face environment (`IBLCubemap`)
4. **Prefab spawn panel** — spawn Cube/Pillar/Orb/Floor/Player from Prefabs UI
5. **AI tools** — `tool_focus`, `tool_select`, `tool_orbit`, `tool_frame`

## +15 more systems

6. Trigger volumes (goal zone enter)
7. PIE + script hot-reload (**F9**)
8. `.mukscene` save/load via console & Content Browser
9. Console commands (`help`, `spawn_prefab`, `save`, `load`, `time`, `snap`)
10. Project settings (snap, fog, day/night)
11. Grid snap on gizmo end + Duplicate (**Ctrl+D**)
12. Selection highlight (gold tint)
13. Camera orbit (**Q/E**)
14. Game timers
15. Localization string table
16. FPS history graph
17. Frame selection toolbar
18. Day/night light factor
19. Content Browser opens scripts/scenes
20. Skin bone CB filled each frame (GPU-ready)

### Controls
| Key | Action |
|-----|--------|
| WASD | Move |
| Q/E | Orbit camera |
| F5 | Play-In-Editor |
| F9 | Hot-reload script |
| F12 | Screenshot |
| Ctrl+Z | Undo (incl. AI batch) |
| Ctrl+D | Duplicate selected |

### Build
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF -DMUK_BUILD_SAMPLES=OFF
cmake --build build --config Release
```

MIT
