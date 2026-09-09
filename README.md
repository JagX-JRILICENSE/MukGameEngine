# Muk Game Engine v0.11

https://github.com/JagX-JRILICENSE/MukGameEngine

## v0.11 — next five

1. **Embedded app icon** — purple/gold Muk logo on the window/taskbar  
2. **AI spawn undo batch** — Ctrl+Z / Toolbar **Undo AI** removes last AI-built entities  
3. **Grid A\* pathfinding** — Enemy chases Player around obstacles  
4. **Bloom + hemisphere IBL** — **Post / IBL** panel (exposure, bloom, sky/ground)  
5. **Skinning** — demo arm skeleton with bone-cube skinning draw  

### Controls
WASD · F5 Play · F12 Screenshot · Ctrl+Z undo (including AI batch)

### Build
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF -DMUK_BUILD_SAMPLES=OFF
cmake --build build --config Release
```

MIT
