# Muk Game Engine v0.6.1

C++20 / DirectX 12 Windows engine.

**Repo:** https://github.com/JagX-JRILICENSE/MukGameEngine

---

## Download Windows app (CI)

1. Open **Actions** → **Windows Build**
2. Open the latest **green** run (or **Run workflow**)
3. Download artifact **`MukGameEngine-Windows-x64`**
4. Unzip → run `bin\MukEditor.exe`

Main CI builds **ImGui + tinygltf** (stable). Jolt is optional/non-blocking.

---

## Free AI (OpenRouter + NVIDIA)

Keys stay on your PC. Defaults use **free** models.

### OpenRouter (free `:free` models)

1. Create a key at https://openrouter.ai/keys  
2. In editor **AI Control** → provider `openrouter` → paste key → **Apply** → **Save**  
3. Click a **FREE** model, e.g.:
   - `nvidia/nemotron-3.5-lightning:free`
   - `nvidia/nemotron-3-ultra-550b-a55b:free`
   - `google/gemma-4-31b-it:free`
   - `openrouter/free`

### NVIDIA (build.nvidia.com)

1. Sign in at https://build.nvidia.com → **Get API Key** (`nvapi-...`)  
2. Provider `nvidia` → paste key → pick e.g. `meta/llama-3.1-8b-instruct`  
3. Base URL: `https://integrate.api.nvidia.com/v1`

Or edit `%APPDATA%\MukGameEngine\settings.ini` (see `config/settings.example.ini`).

---

## Local build

```powershell
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DMUK_USE_IMGUI=ON -DMUK_USE_TINYGLTF=ON -DMUK_USE_JOLT=OFF
cmake --build build --config Release
# Exe: build\bin\Release\MukEditor.exe
```

---

MIT
