#include "Profiler.h"

namespace Muk {

Profiler& Profiler::Get() {
    static Profiler p;
    return p;
}

void Profiler::BeginFrame() {
    m_FrameStart = std::chrono::high_resolution_clock::now();
    m_Scopes.clear();
}

void Profiler::EndFrame() {
    auto end = std::chrono::high_resolution_clock::now();
    m_LastFrameMs = std::chrono::duration<f64, std::milli>(end - m_FrameStart).count();
    m_LastScopes = m_Scopes;
}

void Profiler::Record(const char* name, f64 ms) {
    m_Scopes[name] += ms;
}

ScopedTimer::ScopedTimer(const char* name)
    : m_Name(name), m_Start(std::chrono::high_resolution_clock::now()) {}

ScopedTimer::~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    f64 ms = std::chrono::duration<f64, std::milli>(end - m_Start).count();
    Profiler::Get().Record(m_Name, ms);
}

} // namespace Muk
