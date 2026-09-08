# Muk Game Engine

Modern C++20 / DirectX 12 engine for Windows.

> **Status:** Phase A foundation — **good enough path**, not full Unreal.  
> Differentiation: MIT, lean, **BYOK AI control** (OpenRouter / NVIDIA / OpenAI / custom).

**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine  
**Roadmap:** [ROADMAP.md](ROADMAP.md)

---

## What’s new (Phase A)

- **Lit scene** — directional light + ambient + albedo; roughness/metallic terms in shader
- **ECS world draw** — `Transform` + `MeshRenderer` + `DirectionalLight`
- **Editor** — Viewport RTT, Hierarchy, Details **gizmos** (pos/rot/scale), **Stats** (FPS/ms)
- **AI Control** — BYOK keys; actions `set_camera`, `spawn_cube`
- **Windows CI** — download artifact from Actions

---

## Install

### CI artifact

1. GitHub → **Actions** → **Windows Build** → latest green run  
2. Download **MukGameEngine-Windows-x64** → run `bin\MukEditor.exe`

### Local

```powershell
git clone https://github.com/JagX-JRILICENSE/MukGameEngine.git
cd MukGameEngine
./scripts/build_windows.ps1 -Full
./scripts/package_windows.ps1
```

---

## BYOK AI

Copy `config/settings.example.ini` → `%APPDATA%\MukGameEngine\settings.ini`  
Set `openrouter_api_key` / `nvidia_api_key` / `openai_api_key` (or use the AI panel).

---

## Honest vs Unreal

Muk is **not** Nanite/Lumen/Blueprints/Sequencer.  
It is a **growing** open engine: realtime DX12, editor viewport, ECS, AI hooks.  
See [ROADMAP.md](ROADMAP.md) for “good enough” then “better where it counts.”

---

MIT
