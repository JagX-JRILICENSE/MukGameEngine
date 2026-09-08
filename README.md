# Muk Game Engine

Modern C++20 / DirectX 12 engine.

> **v0.4** — GPU albedo SRVs + HLSL sampling, editor viewport render-to-texture.

**Repo**: https://github.com/JagX-JRILICENSE/MukGameEngine

---

## v0.4 Features

### GPU albedo textures + HLSL sampling
- `DX12TextureCache` uploads RGBA8 → DEFAULT texture + SRV on shared heap
- Root signature: **b0** CBV (MVP + BaseColor + UseTexture), **t0** albedo SRV, **s0** linear wrap sampler
- Pixel shader: `lerp(vertexColor * BaseColor, albedoSample * BaseColor, UseTexture)`
- `Renderer::UploadTexture` + automatic upload when `Material::AlbedoMap` is set
- glTF albedo maps from `LoadGltfFull` can be uploaded the same way

### Editor viewport render-to-texture
- `DX12SceneRT` — offscreen color (RTV+SRV) + depth (DSV)
- `Renderer::EnsureSceneRT` / `BeginSceneRT` / `EndSceneRT`
- Scene draws into RTT; `ImGui::Image` shows GPU SRV in the **Viewport** panel
- Resizes with the docked panel
- After RTT: `BindSwapchainTargets()` so ImGui draws on the swapchain

---

## Build

```bat
scripts\build_windows.bat full
```

or

```bash
cmake .. -G "Visual Studio 17 2022" -A x64 ^
  -DMUK_USE_IMGUI=ON -DMUK_USE_JOLT=ON -DMUK_USE_TINYGLTF=ON
cmake --build . --config Release
```

**MukEditor** (with ImGui): textured triangle in the Viewport panel (SceneRT).  
**MukRuntime**: triangle + cube on the main swapchain with camera MVP.

---

## API sketch

```cpp
// Upload glTF albedo to GPU
auto r = Assets().LoadGltfFull("Assets/model.glb");
if (r.Success) {
    Renderer().UploadMesh("model", *r.MeshData);
    if (r.MaterialData && r.MaterialData->AlbedoMap)
        Renderer().UploadTexture(r.MaterialData->AlbedoTexture, *r.MaterialData->AlbedoMap);
    Renderer().DrawMesh("model", Mat4::Identity(), *r.MaterialData);
}

// Editor viewport
Renderer().EnsureSceneRT(panelW, panelH);
Renderer().BeginSceneRT();
// ... DrawMesh calls ...
Renderer().EndSceneRT();
ImGui::Image(Renderer().GetSceneRTGpuHandle(), size);
```

---

## Layout

```
RHI/DX12/
  DX12RHI        swapchain, depth, SRV heap (128)
  DX12Pipeline   mesh cache, textured PSO
  DX12Texture    albedo GPU upload
  DX12SceneRT    editor viewport RTT
Renderer/        CameraView, UploadTexture, SceneRT API
EditorUI/        Viewport ImGui::Image(SceneRT)
```

---

MIT — **Muk Game Engine**
