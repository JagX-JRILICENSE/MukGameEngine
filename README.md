# Muk Game Engine v0.8

C++20 / DirectX 12 Windows engine with **multi-agent AI game production**.

**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine

---

## Multi-agent AI (OpenRouter + NVIDIA)

Set **both** keys in **AI Control** (free models supported) so agents can specialize:

| Role | Typical provider | Job |
|------|------------------|-----|
| **Architect** | OpenRouter | Design doc, levels, win conditions |
| **Builder** | NVIDIA | Scene ACTION placement |
| **Scripter** | OpenRouter | **Muk Script** gameplay |
| **Critic** | NVIDIA | Verify + demand fixes |

Pipeline: **Architect → Builder → Scripter → UI → Preview → Critic → Fix → Playtest**

### Muk Script (AI-written gameplay)

```text
BEGIN_SCRIPT
set score 0
on_start
  show_ui hud Score:0
on_update dt
  if key W then move Player 0 0 5*dt
  if key A then move Player -5*dt 0 0
  if key D then move Player 5*dt 0 0
  if key S then move Player 0 0 -5*dt
  if score >= 3 then win You win
END_SCRIPT
```

Supports: variables, `if/then`, movement, spawn/destroy, **load_level**, UI, sounds, win/lose.

### How to run

1. Paste OpenRouter **and/or** NVIDIA keys → Save  
2. **AI Game Builder** → describe a full mini-game → **Build complete game with AI**  
3. Watch team log; **Run script now** for longer WASD play  

---

## Engine features (v0.8)

Per-pixel cascades, animation system, spatial audio, content browser, scene JSON, reflection UI, IBL/tonemap, PIE, undo, character, **gameplay VM**, **multi-level runtime**, **dual-provider agents**.

---

## Build / download

Actions → **Windows Build** → `MukGameEngine-Windows-x64`

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON
cmake --build build --config Release
```

MIT
