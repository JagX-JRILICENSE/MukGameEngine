#pragma once
#include "Core/Core.h"

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

#include "Editor/MultiSelect.h"
#include "Renderer/FrustumCulling.h"
#include "Renderer/PostStack.h"
#include "Animation/AnimStateMachine.h"
#include "Navigation/NavMesh.h"
#include "Scene/Timeline.h"
#include "Scene/WorldPartition.h"
#include "Terrain/Heightfield.h"
#include "World/Vegetation.h"
#include "Input/Gamepad.h"
#include "Net/LocalNet.h"
#include "Core/EngineServices.h"

namespace Muk {

struct FeatureHubState {
    MultiSelect Select;
    PostStack Post;
    LODSettings LOD;
    AnimStateMachine AnimSM;
    NavMesh Nav;
    Timeline Cinema;
    WorldPartition Partition;
    Heightfield Terrain;
    WaterPlane Water;
    VegetationScatter Foliage;
    WindSystem Wind;
    Gamepad Pad;
    InputRebind Rebind;
    LocalNet Net;
    Analytics Stats;
    VisualGraph Graph;
    PackageManifest Package;
    bool ShowTerrain = false;
    bool ShowWater = true;
    bool ShowFoliage = true;
    bool FrustumCull = true;
    int CulledCount = 0;
    int DrawnCount = 0;
    int LOD0 = 0, LOD1 = 0, LOD2 = 0;
    float AnimSpeedVar = 0.f;
};

inline void DrawFeatureHub(FeatureHubState& f) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Feature Hub v0.13");
    ImGui::TextWrapped("Production systems: cull, LOD, post, nav, cine, net, graph…");
    if (ImGui::CollapsingHeader("Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Frustum cull", &f.FrustumCull);
        ImGui::Text("Drawn %d | Culled %d | LOD %d/%d/%d",
                    f.DrawnCount, f.CulledCount, f.LOD0, f.LOD1, f.LOD2);
        ImGui::SliderFloat("LOD0 dist", &f.LOD.Lod0, 5, 50);
        ImGui::Checkbox("SSAO", &f.Post.SSAO);
        ImGui::Checkbox("DOF", &f.Post.DOF);
        ImGui::Checkbox("Motion blur", &f.Post.MotionBlur);
        ImGui::SliderFloat("Contrast", &f.Post.Contrast, 0.5f, 2.f);
        ImGui::SliderFloat("Saturation", &f.Post.Saturation, 0, 2);
        ImGui::SliderFloat("Temperature", &f.Post.Temperature, -1, 1);
    }
    if (ImGui::CollapsingHeader("World")) {
        ImGui::Checkbox("Water", &f.ShowWater);
        ImGui::Checkbox("Foliage", &f.ShowFoliage);
        ImGui::SliderFloat("Wind", &f.Wind.Strength, 0, 2);
        ImGui::Text("Foliage: %d  Nav tris: %d", f.Foliage.Count(), f.Nav.TriangleCount());
        ImGui::Text("Chunks %d/%d", f.Partition.LoadedCount(), f.Partition.ChunkCount());
        if (ImGui::Button("Scatter foliage")) f.Foliage.Scatter(80, 20.f);
    }
    if (ImGui::CollapsingHeader("Cinematics")) {
        ImGui::Text("Keys %d  t=%.2f", f.Cinema.KeyCount(), f.Cinema.Time());
        if (ImGui::Button("Demo orbit")) f.Cinema.BuildDemoOrbit();
        ImGui::SameLine();
        if (ImGui::Button("Play")) f.Cinema.Play();
        ImGui::SameLine();
        if (ImGui::Button("Stop")) f.Cinema.Stop();
    }
    if (ImGui::CollapsingHeader("Animation")) {
        ImGui::Text("State: %s", f.AnimSM.Current().c_str());
        if (ImGui::SliderFloat("Speed", &f.AnimSpeedVar, 0, 5))
            f.AnimSM.SetVar("Speed", f.AnimSpeedVar);
    }
    if (ImGui::CollapsingHeader("Input / Net")) {
        auto& p = f.Pad.State();
        ImGui::Text("Pad: %s  stick %.2f,%.2f", p.Connected ? "ON" : "off", p.LeftX, p.LeftY);
        ImGui::Text("Players %d tick %u", (int)f.Net.Players().size(), f.Net.TickIndex());
        if (ImGui::Button("Host")) f.Net.Host("Player1");
        ImGui::SameLine();
        if (ImGui::Button("Add bot")) f.Net.JoinSimulated("Bot");
    }
    if (ImGui::CollapsingHeader("Graph / Package")) {
        ImGui::Text("Nodes: %d  Events: %d", (int)f.Graph.Nodes().size(), (int)f.Stats.Events().size());
        if (ImGui::Button("Demo graph")) f.Graph.BuildDemoCollectibleGraph();
        if (ImGui::Button("Write package.manifest")) {
            f.Package.Version = "0.13.0";
            f.Package.Assets = { "Cube", "Scripts/ai_generated.muk" };
            f.Package.Scenes = { "Scenes/scene.mukscene" };
            f.Package.Save();
        }
    }
    if (ImGui::CollapsingHeader("Multi-select"))
        ImGui::Text("Selected: %d", (int)f.Select.Count());
    ImGui::End();
#else
    (void)f;
#endif
}

} // namespace Muk
