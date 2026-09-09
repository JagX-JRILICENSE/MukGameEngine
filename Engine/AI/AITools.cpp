#include "AITools.h"
#include "Renderer/Renderer.h"
#include "ECS/Component.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace Muk {

AITools::Result AITools::FocusCamera(Renderer& renderer, const Vec3& target, float distance) {
    CameraView cam = renderer.GetCamera();
    cam.Target = target;
    cam.Eye = { target.x + distance * 0.6f, target.y + distance * 0.4f, target.z - distance };
    renderer.SetCamera(cam);
    return { true, "Camera focused" };
}

AITools::Result AITools::SelectByName(std::vector<EditorEntityInfo>& entities, Entity& selected,
                                      const std::string& name) {
    for (auto& info : entities) {
        if (info.Name == name) {
            selected = info.Handle;
            return { true, "Selected " + name };
        }
    }
    return { false, "Entity not found: " + name };
}

AITools::Result AITools::OrbitCamera(Renderer& renderer, float yawDeg, float pitchDeg) {
    CameraView cam = renderer.GetCamera();
    Vec3 toEye{ cam.Eye.x - cam.Target.x, cam.Eye.y - cam.Target.y, cam.Eye.z - cam.Target.z };
    float dist = std::sqrt(toEye.x*toEye.x + toEye.y*toEye.y + toEye.z*toEye.z);
    if (dist < 0.1f) dist = 5.f;
    float yaw = std::atan2(toEye.x, toEye.z) + yawDeg * 0.0174533f;
    float pitch = std::asin(std::clamp(toEye.y / dist, -0.99f, 0.99f)) + pitchDeg * 0.0174533f;
    pitch = std::clamp(pitch, -1.4f, 1.4f);
    cam.Eye = {
        cam.Target.x + dist * std::sin(yaw) * std::cos(pitch),
        cam.Target.y + dist * std::sin(pitch),
        cam.Target.z + dist * std::cos(yaw) * std::cos(pitch)
    };
    renderer.SetCamera(cam);
    return { true, "Orbit applied" };
}

AITools::Result AITools::FrameSelection(Renderer& renderer, World& world, Entity selected) {
    if (!selected.IsValid()) return { false, "Nothing selected" };
    auto* t = world.GetComponent<Transform>(selected);
    if (!t) return { false, "No transform" };
    return FocusCamera(renderer, t->Position, 6.0f);
}

AITools::Result AITools::RunLine(const std::string& line, World& world, Renderer& renderer,
                                 std::vector<EditorEntityInfo>& entities, Entity& selected) {
    if (line.find("tool_focus") != std::string::npos) {
        float x=0,y=0,z=0;
        if (auto p = line.find("x="); p != std::string::npos) sscanf(line.c_str()+p, "x=%f", &x);
        if (auto p = line.find("y="); p != std::string::npos) sscanf(line.c_str()+p, "y=%f", &y);
        if (auto p = line.find("z="); p != std::string::npos) sscanf(line.c_str()+p, "z=%f", &z);
        return FocusCamera(renderer, {x,y,z});
    }
    if (line.find("tool_select") != std::string::npos) {
        char n[64]={};
        if (auto p = line.find("name="); p != std::string::npos) sscanf(line.c_str()+p, "name=%63s", n);
        return SelectByName(entities, selected, n);
    }
    if (line.find("tool_orbit") != std::string::npos) {
        float yaw=15, pitch=0;
        if (auto p = line.find("yaw="); p != std::string::npos) sscanf(line.c_str()+p, "yaw=%f", &yaw);
        if (auto p = line.find("pitch="); p != std::string::npos) sscanf(line.c_str()+p, "pitch=%f", &pitch);
        return OrbitCamera(renderer, yaw, pitch);
    }
    if (line.find("tool_frame") != std::string::npos)
        return FrameSelection(renderer, world, selected);
    return { false, "Unknown tool" };
}

} // namespace Muk
