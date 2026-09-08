#pragma once

#include "Core.h"
#include <chrono>
#include <string>
#include <vector>
#include <unordered_map>

namespace Muk {

class ScopedTimer {
public:
    ScopedTimer(const char* name);
    ~ScopedTimer();
private:
    const char* m_Name;
    std::chrono::high_resolution_clock::time_point m_Start;
};

class Profiler {
public:
    static Profiler& Get();

    void BeginFrame();
    void EndFrame();
    void Record(const char* name, f64 ms);

    f64 LastFrameMs() const { return m_LastFrameMs; }
    f64 Fps() const { return m_LastFrameMs > 0.0 ? 1000.0 / m_LastFrameMs : 0.0; }

    const std::unordered_map<std::string, f64>& LastScopes() const { return m_LastScopes; }

private:
    std::chrono::high_resolution_clock::time_point m_FrameStart;
    f64 m_LastFrameMs = 0.0;
    std::unordered_map<std::string, f64> m_Scopes;
    std::unordered_map<std::string, f64> m_LastScopes;
};

#define MUK_PROFILE_SCOPE(name) ::Muk::ScopedTimer _muk_timer_##__LINE__(name)

} // namespace Muk
