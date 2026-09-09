# Muk Game Engine v0.10

**Product:** Muk Game Engine  
https://github.com/JagX-JRILICENSE/MukGameEngine

## v0.10 features

1. **Non-blocking multi-agent AI** — AsyncAI worker; editor never freezes  
2. **GPU SceneRT screenshots** — F12 → `Assets/Screenshots/*.tga`  
3. **Collectible orbs** — distance pickup, score, complete callback  
4. **Particle bursts** — collect / win FX  
5. **Script autosave** — `Assets/Scripts/*.muk` + Content Browser listing  

### Controls
F5 Play · F12 Screenshot · WASD move · walk into orbs to collect

### Build
```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF -DMUK_BUILD_SAMPLES=OFF
cmake --build build --config Release
```

MIT
