# Muk Game Engine

Modern C++20 / DirectX 12 game engine for Windows.

> **Honest status:** Early foundation (v0.4+). It is **not** full Unreal Engine yet.  
> Goal: grow toward Unreal-class tools/rendering **and** AI-assisted workflows with **bring-your-own-keys**.

**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine

---

## Install on Windows (recommended)

### Option A — Download CI build (no local compile)

1. Open **Actions** on GitHub → workflow **Windows Build**
2. Open the latest successful run
3. Download artifact **`MukGameEngine-Windows-x64`**
4. Unzip → run `bin\MukEditor.exe` or `bin\MukRuntime.exe`

Trigger a build yourself: **Actions → Windows Build → Run workflow**

### Option B — Build locally

```powershell
git clone https://github.com/JagX-JRILICENSE/MukGameEngine.git
cd MukGameEngine
./scripts/build_windows.ps1 -Full
./scripts/package_windows.ps1
```

Requires: Windows 10/11, VS 2022 (C++), CMake 3.25+, Git

---

## Realtime preview (what you get today)

| App | What you see |
|-----|----------------|
| **MukEditor** | Docked UI, **Viewport** with live SceneRT (triangle + cube, depth, textured demo), Hierarchy/Details, **AI Control** panel |
| **MukRuntime** | Game-style loop: camera MVP, multi-mesh, physics step |

This is a **working realtime DX12 loop**, not a static mockup.

---

## Bring Your Own API Key (BYOK)

Control / advise the engine with **your** keys — no vendor lock-in.

Supported providers (OpenAI-compatible):

- **OpenRouter**
- **NVIDIA** Integrate API (`integrate.api.nvidia.com`)
- **OpenAI**
- **Custom** base URL + key + model

### Setup

1. Copy `config/settings.example.ini` → `%APPDATA%\MukGameEngine\settings.ini`
2. Set e.g. `provider=openrouter` and `openrouter_api_key=sk-or-...`
3. Or paste the key in the editor **AI Control (BYOK)** panel and **Save settings**

Keys stay on your machine; requests go only to the provider you choose.

### AI Control panel

- Chat with the model about systems / levels / code
- Model can emit `ACTION: set_camera eye=... target=...` to move the live camera
- More actions (spawn, materials, etc.) will expand over time

---

## What works now vs Unreal

| Area | Muk today | Unreal |
|------|-----------|--------|
| DX12 triangle/cube, depth, textures | Yes | Yes |
| Editor docking + viewport RTT | Yes (early) | Full |
| Physics | Simple (+ optional Jolt path) | Chaos |
| glTF | Mesh + material + CPU/GPU albedo path | Full pipeline |
| Blueprints / sequencer / Nanite / Lumen | Not yet | Yes |
| BYOK AI control | Yes (early) | Plugins / external |

Roadmap aims at high-end rendering, full editor, animation, audio, networking, **and** deeper AI actions.

---

## CI

`.github/workflows/windows-build.yml` builds Release on `windows-latest` with ImGui + tinygltf and uploads a zip-ready artifact.

---

## License

MIT
