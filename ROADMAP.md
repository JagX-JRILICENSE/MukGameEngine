# Muk Roadmap — Good Enough, Then Better (Where It Counts)

**Truth:** Matching all of Unreal is multi-year. Beating Unreal for *everyone* is unrealistic short-term.  
**Strategy:** Become **good enough** to ship real games/tools, then **better** on openness, speed, and AI-native control.

---

## Phase A — Good enough foundation (in progress)

| Item | Status |
|------|--------|
| DX12 mesh + depth + textures | Done |
| Editor viewport RTT | Done |
| BYOK AI (OpenRouter / NVIDIA / OpenAI / custom) | Done |
| Windows CI package | Done |
| **Lit shading (Lambert + ambient + roughness/metal terms)** | **Done** |
| **ECS scene draw (MeshRenderer + Transform)** | **Done** |
| **DirectionalLight component** | **Done** |
| **Transform gizmos (Details panel)** | **Done** |
| **Frame profiler / Stats panel** | **Done** |
| **AI actions: set_camera, spawn_cube** | **Done** |
| Shadow maps | Next |
| Full Jolt characters/constraints | Next |
| Undo stack | Next |

## Phase B — Credible editor

- Viewport translate/rotate/scale gizmos (ImGuizmo)
- Content Browser + asset registry
- Play-in-editor
- Reflection-driven properties
- Prefabs

## Phase C — Production slice

- Animation (skinned meshes)
- Audio
- Navmesh
- Save/load worlds
- Packaging installer

## Phase D — High-end graphics

- Cascaded shadows, PBR IBL
- Post stack (TAA, bloom, exposure)
- Mesh LODs / streaming

## Phase E — Moat (better than Unreal *for some users*)

- Deep AI scene ops (materials, graphs, tests)
- MIT + no royalty
- Minute-scale engine rebuilds
- Transparent, small codebase

---

## Success metrics

1. **Good enough:** Indie can ship a small 3D game without fighting the engine.  
2. **Better:** Faster iteration + AI control + zero license tax vs UE for that team.
