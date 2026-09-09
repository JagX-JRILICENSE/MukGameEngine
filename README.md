# Muk Game Engine v0.13

https://github.com/JagX-JRILICENSE/MukGameEngine

Open DX12 C++ engine with an **AI-native editor**. Not a claim that every Unreal/Unity subsystem is matched — it is a fast, modern stack plus capabilities those tools do not ship by default (multi-provider AI game builder, async agents, undoable AI spawns).

## Why Muk is competitive

| Area | Muk advantage |
|------|----------------|
| AI | Multi-agent OpenRouter + NVIDIA build/playtest/fix loop in-editor |
| Weight | Single static lib + editor exe; no multi-GB launcher |
| Scripting | Muk Script + visual graph data model + hot reload (F9) |
| Iteration | AI spawn undo, async AI, package.manifest, analytics |
| Rendering | Cascades, IBL cubemap, bloom, SSAO/DOF/grade settings, frustum+LOD |
| World | Navmesh, partition streaming, foliage+wind, water, terrain heightfield |
| Cinema | Timeline camera sequencer |
| Input | XInput gamepad + rebind map |
| Net | Local-authority multiplayer scaffold |

## v0.13 systems (~40 upgrades)

**Rendering:** frustum cull, LOD tiers, SSAO/DOF/motion blur/color grade stack, selection multi-tint, water plane, wind-animated foliage  
**Animation:** state machine, two-bone IK, GPU SkinCB + Skin.hlsl  
**World:** navmesh bake, world partition, heightfield terrain, vegetation scatter  
**Editor:** Feature Hub panel, multi-select, cinematic timeline, package manifest  
**Input/Net:** gamepad, rebind, local net host/bots  
**Ops:** crash log, analytics events, visual script graph  
**ECS:** Tag/Layer/Billboard/LOD radius components  

Open **Feature Hub v0.13** in the editor for live controls.

### Build
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF -DMUK_BUILD_SAMPLES=OFF
cmake --build build --config Release
```

MIT
