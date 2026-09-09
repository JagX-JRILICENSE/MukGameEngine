# Muk Game Engine v0.14

https://github.com/JagX-JRILICENSE/MukGameEngine

## Honest positioning

Muk is **not** Unreal Engine. Unreal has decades of production tooling, AAA rendering, Marketplace, and ecosystem scale we do not claim to match overnight.

What Muk *does* ship that is rare or stronger for small teams:

- **AI-native game builder** (multi-provider, async, undoable spawns, script gen, playtest loop)
- **Full source, tiny footprint**, one DX12 editor exe
- **Modern core**: cascades, GPU skin PSO, fullscreen SSAO+bloom, nav, partition, net

## v0.14 headline systems

| Requested | Status |
|-----------|--------|
| Full GPU skin PSO | `DX12SkinPSO` — bone CB + skinned cube draw path |
| Real SSAO + bloom fullscreen | `DX12PostFX` fullscreen triangle pass |
| Node-graph canvas UI | ImGui **Node Graph** panel (drag nodes, links) |
| UDP multiplayer | `UdpNet` host/connect, transform packets |
| Mature animation tooling | Montages, notifies, layers, 1D blend tree |
| Ecosystem size | `PluginRegistry` + 10 builtin modules + samples |

### +15 gameplay systems
Health, Inventory, Quests, Dialogue, Save slots, Weather, Minimap, Achievements, Combo, Vehicle, Build grid, Wallet, Stealth, Wave spawner (+ existing triggers/collectibles)

### Build
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF -DMUK_BUILD_SAMPLES=OFF
cmake --build build --config Release
```

MIT
