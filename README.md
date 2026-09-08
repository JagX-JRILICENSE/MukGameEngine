# Muk Game Engine v0.9

**Product name:** Muk Game Engine  
**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine

---

## Download Windows app

**Actions → Windows Build → artifact `MukGameEngine-Windows-x64`**

Run `bin\Muk Game Engine.exe` or `MukEditor.exe`.

---

## v0.9 upgrades

### Pipeline / AI
- **Distance triggers** — `proximity Player Orb1 1.5` + `on_trigger` for orb pickup
- **Async AI** — background thread job queue (non-blocking editor)
- **Autosave scripts** → `Assets/Scripts/last_generated.muk`
- Multi-agent OpenRouter + NVIDIA team still first-class

### Five new systems
1. **Input action map** — named binds (MoveForward, Screenshot, …)
2. **Prefabs** — Cube / Pillar / Orb / FloorTile / PlayerCapsule
3. **Screenshots** — F12 → `Assets/Screenshots/*.tga`
4. **Script hot-reload** — F9 reloads last generated Muk Script
5. **App branding** — window title **Muk Game Engine**, VERSIONINFO resource

### Controls
| Key | Action |
|-----|--------|
| F5 | Play-In-Editor |
| F9 | Reload gameplay script |
| F12 | Screenshot |
| WASD | Move / script move |
| Ctrl+Z/Y | Undo/Redo |

---

## Local build

```powershell
cmake -B build -A x64 -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF -DMUK_BUILD_SAMPLES=OFF
cmake --build build --config Release
```

MIT
