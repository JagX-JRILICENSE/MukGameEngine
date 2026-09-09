#pragma once
#include "Core/Core.h"
#include "Skeleton.h"
#include "AnimStateMachine.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace Muk {

struct AnimNotify {
    float Time = 0;
    std::string Name;
    bool Fired = false;
};

struct AnimMontage {
    std::string Name;
    std::string ClipName;
    float PlayRate = 1.f;
    float BlendIn = 0.1f;
    float BlendOut = 0.15f;
    bool Loop = false;
    std::vector<AnimNotify> Notifies;
};

struct AnimLayer {
    std::string Name;
    std::string ClipName;
    float Weight = 1.f;
    float Time = 0;
    bool Additive = false;
};

/** Blend tree node: 1D blend by parameter */
struct BlendNode1D {
    std::string Param;
    struct Entry { float Value = 0; std::string Clip; };
    std::vector<Entry> Entries;

    std::string Evaluate(float param) const {
        if (Entries.empty()) return {};
        if (Entries.size() == 1) return Entries[0].Clip;
        // find segment
        for (size_t i = 0; i + 1 < Entries.size(); ++i) {
            if (param >= Entries[i].Value && param <= Entries[i+1].Value) {
                float t = (param - Entries[i].Value) / std::max(1e-5f, Entries[i+1].Value - Entries[i].Value);
                return t < 0.5f ? Entries[i].Clip : Entries[i+1].Clip;
            }
        }
        return param < Entries.front().Value ? Entries.front().Clip : Entries.back().Clip;
    }
};

class AnimationTooling {
public:
    void RegisterMontage(const AnimMontage& m) { m_Montages[m.Name] = m; }
    void PlayMontage(const std::string& name) {
        auto it = m_Montages.find(name);
        if (it == m_Montages.end()) return;
        m_Active = it->second;
        m_ActiveTime = 0;
        m_Playing = true;
        for (auto& n : m_Active.Notifies) n.Fired = false;
    }

    void AddLayer(const AnimLayer& l) { m_Layers.push_back(l); }
    void SetBlendTree(const BlendNode1D& b) { m_Blend = b; }

    void Update(float dt, std::vector<std::string>& outNotifies) {
        if (m_Playing) {
            m_ActiveTime += dt * m_Active.PlayRate;
            for (auto& n : m_Active.Notifies) {
                if (!n.Fired && m_ActiveTime >= n.Time) {
                    n.Fired = true;
                    outNotifies.push_back(n.Name);
                }
            }
            // assume 1s default clip length if not loop
            if (!m_Active.Loop && m_ActiveTime > 1.0f) m_Playing = false;
        }
        for (auto& l : m_Layers) l.Time += dt;
    }

    bool IsMontagePlaying() const { return m_Playing; }
    const std::string& ActiveClip() const { return m_Active.ClipName; }
    float ActiveTime() const { return m_ActiveTime; }

    std::string EvaluateLocomotion(float speed) const {
        return m_Blend.Evaluate(speed);
    }

    void BuildDefaultLocomotion() {
        BlendNode1D b;
        b.Param = "Speed";
        b.Entries = { {0.f, "Idle"}, {1.f, "Walk"}, {3.f, "Run"}, {6.f, "Sprint"} };
        m_Blend = b;

        AnimMontage jump;
        jump.Name = "Jump"; jump.ClipName = "Wave"; jump.PlayRate = 1.2f;
        jump.Notifies = { {0.1f, "JumpStart"}, {0.5f, "JumpApex"}, {0.9f, "JumpLand"} };
        RegisterMontage(jump);

        AnimMontage attack;
        attack.Name = "Attack"; attack.ClipName = "Wave"; attack.PlayRate = 1.5f;
        attack.Notifies = { {0.3f, "HitWindow"} };
        RegisterMontage(attack);
    }

    size_t MontageCount() const { return m_Montages.size(); }
    size_t LayerCount() const { return m_Layers.size(); }

private:
    std::unordered_map<std::string, AnimMontage> m_Montages;
    AnimMontage m_Active{};
    float m_ActiveTime = 0;
    bool m_Playing = false;
    std::vector<AnimLayer> m_Layers;
    BlendNode1D m_Blend{};
};

} // namespace Muk
