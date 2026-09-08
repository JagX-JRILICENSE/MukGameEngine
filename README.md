# Muk Game Engine v0.7

C++20 / DirectX 12 Windows engine with AI-assisted building.

**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine

---

## v0.7 highlights

| Area | Status |
|------|--------|
| **Per-pixel cascades** | 3 LightVPs + splits in CB; distance pick + soft PCF |
| **Skinned animation** | Skeleton / clips / Animator + demo Wave arm |
| **Spatial audio** | XAudio2 init + tone clips + distance attenuation |
| **Content Browser** | Scan Assets/, import/reimport glTF |
| **Save / load world** | JSON scene serializer |
| **Reflection UI** | Generic Details for Transform/Mesh/Light/Camera |
| **PBR-ish look** | Hemisphere IBL + exposure Reinhard tonemap |
| **AI Game Builder** | Plan → build → preview → verify → fix loop |

---

## AI Game Builder (autonomous)

1. Set OpenRouter/NVIDIA **API key** in **AI Control** (free models supported)
2. Open **AI Game Builder**
3. Describe a scene/game (e.g. *“arena with floor, 4 pillars, player cube”*)
4. Click **Build with AI**

The agent:
- plans with ACTION lines
- spawns meshes / sets camera / light / materials
- moves “cursor” (`select` + `focus_camera`)
- frames a **preview**
- **verifies** entity counts / floor / issues
- **fixes** and retries (up to 4)

This is an **assistive designer**, not a full game programmer. It builds layouts and iterates; gameplay scripts still need you.

---

## Download Windows app

**Actions** → **Windows Build** → artifact `MukGameEngine-Windows-x64` → `bin\MukEditor.exe`

---

## Local build

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF
cmake --build build --config Release
```

---

MIT
