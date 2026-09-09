#pragma once
#include "Core/EngineServices.h"
#include <cmath>

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

/** Interactive Blueprint-style node canvas */
inline void DrawNodeGraphCanvas(VisualGraph& graph) {
#ifdef MUK_USE_IMGUI
    ImGui::Begin("Node Graph");
    if (ImGui::Button("Add Event")) graph.AddNode("Event", "OnUpdate", 40.f + graph.Nodes().size()*20.f, 60.f);
    ImGui::SameLine();
    if (ImGui::Button("Add Spawn")) graph.AddNode("Spawn", "Spawn", 200.f, 60.f);
    ImGui::SameLine();
    if (ImGui::Button("Add Branch")) graph.AddNode("Branch", "If", 360.f, 60.f);
    ImGui::SameLine();
    if (ImGui::Button("Demo")) graph.BuildDemoCollectibleGraph();

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImGui::GetContentRegionAvail();
    if (canvasSize.x < 100) canvasSize.x = 400;
    if (canvasSize.y < 100) canvasSize.y = 300;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                      IM_COL32(30, 30, 36, 255));

    // Grid
    for (float x = fmodf(canvasPos.x, 32.f); x < canvasSize.x; x += 32.f)
        dl->AddLine(ImVec2(canvasPos.x + x, canvasPos.y), ImVec2(canvasPos.x + x, canvasPos.y + canvasSize.y), IM_COL32(50,50,55,255));
    for (float y = fmodf(canvasPos.y, 32.f); y < canvasSize.y; y += 32.f)
        dl->AddLine(ImVec2(canvasPos.x, canvasPos.y + y), ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + y), IM_COL32(50,50,55,255));

    // Links first
    for (auto& n : graph.Nodes()) {
        for (int outId : n.Outs) {
            for (auto& m : graph.Nodes()) {
                if (m.Id != outId) continue;
                ImVec2 a(canvasPos.x + n.X + 120, canvasPos.y + n.Y + 20);
                ImVec2 b(canvasPos.x + m.X, canvasPos.y + m.Y + 20);
                dl->AddBezierCubic(a, ImVec2(a.x+40, a.y), ImVec2(b.x-40, b.y), b, IM_COL32(120, 180, 255, 200), 2.f);
            }
        }
    }

    // Nodes (draggable)
    for (auto& n : const_cast<std::vector<GraphNode>&>(graph.Nodes())) {
        ImGui::PushID(n.Id);
        ImGui::SetCursorScreenPos(ImVec2(canvasPos.x + n.X, canvasPos.y + n.Y));
        ImGui::BeginChild("node", ImVec2(130, 48), true);
        ImU32 col = IM_COL32(70, 90, 140, 255);
        if (n.Type == "Event") col = IM_COL32(140, 70, 70, 255);
        if (n.Type == "Spawn") col = IM_COL32(70, 140, 90, 255);
        if (n.Type == "Branch") col = IM_COL32(140, 120, 50, 255);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, col);
        ImGui::Text("%s", n.Title.c_str());
        ImGui::TextDisabled("%s #%d", n.Type.c_str(), n.Id);
        ImGui::PopStyleColor();
        if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            n.X += ImGui::GetIO().MouseDelta.x;
            n.Y += ImGui::GetIO().MouseDelta.y;
        }
        ImGui::EndChild();
        ImGui::PopID();
    }

    ImGui::SetCursorScreenPos(ImVec2(canvasPos.x, canvasPos.y + canvasSize.y + 4));
    ImGui::Text("Nodes: %d — drag to move", (int)graph.Nodes().size());
    ImGui::End();
#else
    (void)graph;
#endif
}

} // namespace Muk
