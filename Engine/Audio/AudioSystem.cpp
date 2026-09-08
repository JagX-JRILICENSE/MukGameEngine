#include "AudioSystem.h"
#include "Core/Log.h"
#include <cmath>
#include <algorithm>

#ifdef MUK_PLATFORM_WINDOWS
#include <xaudio2.h>
#pragma comment(lib, "xaudio2.lib")
#endif

namespace Muk {

bool AudioSystem::Initialize() {
#ifdef MUK_PLATFORM_WINDOWS
    IXAudio2* xa = nullptr;
    if (FAILED(XAudio2Create(&xa, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        MUK_CORE_WARN("XAudio2Create failed — audio disabled");
        return false;
    }
    IXAudio2MasteringVoice* master = nullptr;
    if (FAILED(xa->CreateMasteringVoice(&master))) {
        xa->Release();
        MUK_CORE_WARN("Mastering voice failed");
        return false;
    }
    m_XAudio = xa;
    m_Mastering = master;
    m_Ready = true;
    RegisterTone("beep", 880.0f, 0.12f, 0.25f);
    RegisterTone("place", 440.0f, 0.08f, 0.2f);
    RegisterTone("error", 220.0f, 0.2f, 0.3f);
    RegisterTone("success", 660.0f, 0.15f, 0.25f);
    MUK_CORE_INFO("AudioSystem ready (XAudio2 + spatial tones)");
    return true;
#else
    m_Ready = true;
    RegisterTone("beep", 880.0f, 0.12f, 0.25f);
    return true;
#endif
}

void AudioSystem::Shutdown() {
    StopAll();
#ifdef MUK_PLATFORM_WINDOWS
    if (m_Mastering) {
        static_cast<IXAudio2MasteringVoice*>(m_Mastering)->DestroyVoice();
        m_Mastering = nullptr;
    }
    if (m_XAudio) {
        static_cast<IXAudio2*>(m_XAudio)->Release();
        m_XAudio = nullptr;
    }
#endif
    m_Clips.clear();
    m_Ready = false;
}

bool AudioSystem::RegisterTone(const std::string& name, f32 frequencyHz, f32 durationSec, f32 volume) {
    Clip c;
    c.SampleRate = 44100;
    const int n = (int)(durationSec * c.SampleRate);
    c.Samples.resize(std::max(1, n));
    for (int i = 0; i < n; ++i) {
        f32 t = (f32)i / (f32)c.SampleRate;
        f32 env = 1.0f - (f32)i / (f32)std::max(1, n); // simple decay
        c.Samples[i] = std::sin(2.0f * 3.14159265f * frequencyHz * t) * volume * env;
    }
    m_Clips[name] = std::move(c);
    return true;
}

f32 AudioSystem::ComputeAttenuation(const Voice& v) const {
    if (!v.Spatial) return v.Volume;
    Vec3 d = {
        v.Position.x - m_Listener.Position.x,
        v.Position.y - m_Listener.Position.y,
        v.Position.z - m_Listener.Position.z
    };
    f32 dist = std::sqrt(d.x*d.x + d.y*d.y + d.z*d.z);
    if (dist <= v.MinD) return v.Volume;
    if (dist >= v.MaxD) return 0.0f;
    f32 t = (dist - v.MinD) / (v.MaxD - v.MinD);
    return v.Volume * (1.0f - t);
}

u32 AudioSystem::Play(const AudioSourceDesc& desc) {
    if (!m_Clips.count(desc.ClipName)) return 0;
    Voice v;
    v.Id = m_NextId++;
    v.ClipName = desc.ClipName;
    v.Position = desc.Position;
    v.Volume = desc.Volume;
    v.MinD = desc.MinDistance;
    v.MaxD = desc.MaxDistance;
    v.Loop = desc.Loop;
    v.Spatial = desc.Spatial;
    v.Cursor = 0;
    v.Active = true;
    m_Voices.push_back(v);
    return v.Id;
}

void AudioSystem::Stop(u32 voiceId) {
    for (auto& v : m_Voices)
        if (v.Id == voiceId) v.Active = false;
}

void AudioSystem::StopAll() {
    m_Voices.clear();
}

void AudioSystem::Update(f32 dt) {
    (void)dt;
    // Software mixer stub: advance cursors, drop finished
    // Full XAudio source voices can be wired later; attenuation is computed for game logic
    for (auto& v : m_Voices) {
        if (!v.Active) continue;
        auto it = m_Clips.find(v.ClipName);
        if (it == m_Clips.end()) { v.Active = false; continue; }
        const auto& clip = it->second;
        v.Cursor += clip.SampleRate * dt;
        if (v.Cursor >= (f32)clip.Samples.size()) {
            if (v.Loop) v.Cursor = std::fmod(v.Cursor, (f32)clip.Samples.size());
            else v.Active = false;
        }
        (void)ComputeAttenuation(v);
    }
    m_Voices.erase(std::remove_if(m_Voices.begin(), m_Voices.end(),
        [](const Voice& v) { return !v.Active; }), m_Voices.end());
}

} // namespace Muk
