#pragma once
#include "Math/Vector.h"
#include "Renderer/Renderer.h"
#include <string>
#include <vector>
#include <algorithm>

namespace Muk {

struct CameraKey {
    float Time = 0;
    Vec3 Eye{};
    Vec3 Target{};
    float FOV = 60.f;
};

class Timeline {
public:
    void Clear() { m_Keys.clear(); m_Time = 0; m_Playing = false; }
    void AddKey(const CameraKey& k) {
        m_Keys.push_back(k);
        std::sort(m_Keys.begin(), m_Keys.end(),
                  [](const CameraKey& a, const CameraKey& b) { return a.Time < b.Time; });
    }
    void Play() { m_Playing = true; m_Time = 0; }
    void Stop() { m_Playing = false; }
    bool IsPlaying() const { return m_Playing; }
    float Time() const { return m_Time; }
    float Duration() const {
        return m_Keys.empty() ? 0.f : m_Keys.back().Time;
    }

    void Update(float dt, Renderer& renderer) {
        if (!m_Playing || m_Keys.size() < 2) return;
        m_Time += dt;
        if (m_Time >= Duration()) { m_Playing = false; m_Time = Duration(); }
        // lerp between keys
        for (size_t i = 0; i + 1 < m_Keys.size(); ++i) {
            if (m_Time >= m_Keys[i].Time && m_Time <= m_Keys[i+1].Time) {
                float t0 = m_Keys[i].Time, t1 = m_Keys[i+1].Time;
                float u = (t1 > t0) ? (m_Time - t0) / (t1 - t0) : 0;
                auto lerp = [](const Vec3& a, const Vec3& b, float u) {
                    return Vec3{ a.x+(b.x-a.x)*u, a.y+(b.y-a.y)*u, a.z+(b.z-a.z)*u };
                };
                CameraView cam;
                cam.Eye = lerp(m_Keys[i].Eye, m_Keys[i+1].Eye, u);
                cam.Target = lerp(m_Keys[i].Target, m_Keys[i+1].Target, u);
                renderer.SetCamera(cam);
                return;
            }
        }
    }

    void BuildDemoOrbit() {
        Clear();
        AddKey({ 0, { 8, 4, -10 }, { 0, 0.5f, 0 } });
        AddKey({ 2, { 0, 6, -12 }, { 0, 0.5f, 0 } });
        AddKey({ 4, { -8, 4, -6 }, { 0, 0.5f, 0 } });
        AddKey({ 6, { 8, 4, -10 }, { 0, 0.5f, 0 } });
    }

    int KeyCount() const { return (int)m_Keys.size(); }

private:
    std::vector<CameraKey> m_Keys;
    float m_Time = 0;
    bool m_Playing = false;
};

} // namespace Muk
