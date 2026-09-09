#pragma once
#include "Core/Core.h"
#include "Skeleton.h"
#include "Math/Vector.h"
#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>

namespace Muk {

struct AnimTransition {
    std::string ToState;
    std::string ConditionVar;
    float Threshold = 0.5f;
    bool Greater = true;
    float BlendTime = 0.15f;
};

struct AnimState {
    std::string Name;
    std::string ClipName;
    float Speed = 1.f;
    std::vector<AnimTransition> Transitions;
};

class AnimStateMachine {
public:
    void AddState(const AnimState& s) { m_States[s.Name] = s; }
    void SetVar(const std::string& n, float v) { m_Vars[n] = v; }
    float GetVar(const std::string& n, float d = 0) const {
        auto it = m_Vars.find(n); return it == m_Vars.end() ? d : it->second;
    }
    void SetState(const std::string& n) {
        if (m_States.count(n)) { m_Current = n; m_Time = 0; }
    }
    const std::string& Current() const { return m_Current; }
    std::string CurrentClip() const {
        auto it = m_States.find(m_Current);
        return it == m_States.end() ? "" : it->second.ClipName;
    }

    void Update(float dt) {
        m_Time += dt;
        auto it = m_States.find(m_Current);
        if (it == m_States.end()) return;
        for (auto& tr : it->second.Transitions) {
            float v = GetVar(tr.ConditionVar);
            bool ok = tr.Greater ? (v > tr.Threshold) : (v < tr.Threshold);
            if (ok) { SetState(tr.ToState); return; }
        }
    }

    void BuildLocomotionDemo() {
        AnimState idle{ "Idle", "Wave", 1.f };
        AnimState run{ "Run", "Wave", 1.5f };
        idle.Transitions.push_back({ "Run", "Speed", 0.1f, true, 0.2f });
        run.Transitions.push_back({ "Idle", "Speed", 0.1f, false, 0.2f });
        AddState(idle); AddState(run);
        SetState("Idle");
    }

private:
    std::unordered_map<std::string, AnimState> m_States;
    std::unordered_map<std::string, float> m_Vars;
    std::string m_Current = "Idle";
    float m_Time = 0;
};

struct TwoBoneIK {
    static void Solve(Vec3 root, float lenA, float lenB, const Vec3& target,
                      Vec3& midOut, Vec3& endOut) {
        Vec3 to = { target.x - root.x, target.y - root.y, target.z - root.z };
        float dist = std::sqrt(to.x*to.x + to.y*to.y + to.z*to.z);
        dist = std::max(0.01f, std::min(dist, lenA + lenB - 0.001f));
        float cosA = (lenA*lenA + dist*dist - lenB*lenB) / (2.f * lenA * dist);
        cosA = std::clamp(cosA, -1.f, 1.f);
        float angle = std::acos(cosA);
        Vec3 dir = { to.x / dist, to.y / dist, to.z / dist };
        Vec3 up{ 0, 1, 0 };
        Vec3 side{ dir.y*up.z - dir.z*up.y, dir.z*up.x - dir.x*up.z, dir.x*up.y - dir.y*up.x };
        float sl = std::sqrt(side.x*side.x+side.y*side.y+side.z*side.z);
        if (sl > 1e-5f) { side.x/=sl; side.y/=sl; side.z/=sl; }
        else side = { 1, 0, 0 };
        float bend = std::sin(angle) * lenA;
        midOut = { root.x + dir.x * std::cos(angle) * lenA + side.x * bend,
                   root.y + dir.y * std::cos(angle) * lenA + side.y * bend,
                   root.z + dir.z * std::cos(angle) * lenA + side.z * bend };
        endOut = { root.x + dir.x * dist, root.y + dir.y * dist, root.z + dir.z * dist };
    }
};

} // namespace Muk
