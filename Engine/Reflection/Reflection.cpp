#include "Reflection.h"

#ifdef MUK_USE_IMGUI
#include <imgui.h>
#endif

namespace Muk {

std::vector<ComponentView> Reflection::Inspect(World& world, Entity e) {
    std::vector<ComponentView> views;
    if (!e.IsValid()) return views;

    if (auto* n = world.GetComponent<NameComponent>(e)) {
        ComponentView v;
        v.TypeName = "NameComponent";
        PropertyDesc p;
        p.Name = "Name";
        p.Type = PropType::String;
        p.GetString = [n]() { return n->Name; };
        p.SetString = [n](const std::string& s) { n->Name = s; };
        v.Properties.push_back(p);
        views.push_back(v);
    }

    if (auto* t = world.GetComponent<Transform>(e)) {
        ComponentView v;
        v.TypeName = "Transform";
        {
            PropertyDesc p; p.Name = "Position"; p.Type = PropType::Float3;
            p.GetFloat3 = [t](float* o) { o[0]=t->Position.x; o[1]=t->Position.y; o[2]=t->Position.z; };
            p.SetFloat3 = [t](const float* o) { t->Position = {o[0],o[1],o[2]}; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Rotation"; p.Type = PropType::Float3;
            p.GetFloat3 = [t](float* o) { o[0]=t->Rotation.x; o[1]=t->Rotation.y; o[2]=t->Rotation.z; };
            p.SetFloat3 = [t](const float* o) { t->Rotation = {o[0],o[1],o[2]}; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Scale"; p.Type = PropType::Float3;
            p.GetFloat3 = [t](float* o) { o[0]=t->Scale.x; o[1]=t->Scale.y; o[2]=t->Scale.z; };
            p.SetFloat3 = [t](const float* o) { t->Scale = {o[0],o[1],o[2]}; };
            v.Properties.push_back(p);
        }
        views.push_back(v);
    }

    if (auto* mr = world.GetComponent<MeshRenderer>(e)) {
        ComponentView v;
        v.TypeName = "MeshRenderer";
        {
            PropertyDesc p; p.Name = "Mesh"; p.Type = PropType::String;
            p.GetString = [mr]() { return mr->MeshName; };
            p.SetString = [mr](const std::string& s) { mr->MeshName = s; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Material"; p.Type = PropType::String;
            p.GetString = [mr]() { return mr->MaterialName; };
            p.SetString = [mr](const std::string& s) { mr->MaterialName = s; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Visible"; p.Type = PropType::Bool;
            p.GetBool = [mr]() { return mr->Visible; };
            p.SetBool = [mr](bool b) { mr->Visible = b; };
            v.Properties.push_back(p);
        }
        views.push_back(v);
    }

    if (auto* c = world.GetComponent<Camera>(e)) {
        ComponentView v;
        v.TypeName = "Camera";
        {
            PropertyDesc p; p.Name = "FOV"; p.Type = PropType::Float;
            p.GetFloat = [c](float* o) { *o = c->FOV; };
            p.SetFloat = [c](const float* o) { c->FOV = *o; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Near"; p.Type = PropType::Float;
            p.GetFloat = [c](float* o) { *o = c->Near; };
            p.SetFloat = [c](const float* o) { c->Near = *o; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Far"; p.Type = PropType::Float;
            p.GetFloat = [c](float* o) { *o = c->Far; };
            p.SetFloat = [c](const float* o) { c->Far = *o; };
            v.Properties.push_back(p);
        }
        views.push_back(v);
    }

    if (auto* L = world.GetComponent<DirectionalLight>(e)) {
        ComponentView v;
        v.TypeName = "DirectionalLight";
        {
            PropertyDesc p; p.Name = "Direction"; p.Type = PropType::Float3;
            p.GetFloat3 = [L](float* o) { o[0]=L->Direction.x; o[1]=L->Direction.y; o[2]=L->Direction.z; };
            p.SetFloat3 = [L](const float* o) { L->Direction = {o[0],o[1],o[2]}; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Color"; p.Type = PropType::Float3;
            p.GetFloat3 = [L](float* o) { o[0]=L->Color.x; o[1]=L->Color.y; o[2]=L->Color.z; };
            p.SetFloat3 = [L](const float* o) { L->Color = {o[0],o[1],o[2]}; };
            v.Properties.push_back(p);
        }
        {
            PropertyDesc p; p.Name = "Intensity"; p.Type = PropType::Float;
            p.GetFloat = [L](float* o) { *o = L->Intensity; };
            p.SetFloat = [L](const float* o) { L->Intensity = *o; };
            v.Properties.push_back(p);
        }
        views.push_back(v);
    }

    return views;
}

bool Reflection::DrawImGui(World& world, Entity e) {
#ifdef MUK_USE_IMGUI
    bool changed = false;
    auto views = Inspect(world, e);
    for (auto& v : views) {
        if (ImGui::CollapsingHeader(v.TypeName.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& p : v.Properties) {
                if (p.Type == PropType::Float3 && p.GetFloat3 && p.SetFloat3) {
                    float v3[3];
                    p.GetFloat3(v3);
                    if (ImGui::DragFloat3(p.Name.c_str(), v3, 0.05f)) {
                        p.SetFloat3(v3);
                        changed = true;
                    }
                } else if (p.Type == PropType::Float && p.GetFloat && p.SetFloat) {
                    float f = 0;
                    p.GetFloat(&f);
                    if (ImGui::DragFloat(p.Name.c_str(), &f, 0.05f)) {
                        p.SetFloat(&f);
                        changed = true;
                    }
                } else if (p.Type == PropType::Bool && p.GetBool && p.SetBool) {
                    bool b = p.GetBool();
                    if (ImGui::Checkbox(p.Name.c_str(), &b)) {
                        p.SetBool(b);
                        changed = true;
                    }
                } else if (p.Type == PropType::String && p.GetString && p.SetString) {
                    std::string s = p.GetString();
                    char buf[128];
                    std::snprintf(buf, sizeof(buf), "%s", s.c_str());
                    if (ImGui::InputText(p.Name.c_str(), buf, sizeof(buf))) {
                        p.SetString(buf);
                        changed = true;
                    }
                }
            }
        }
    }
    return changed;
#else
    (void)world; (void)e;
    return false;
#endif
}

} // namespace Muk
