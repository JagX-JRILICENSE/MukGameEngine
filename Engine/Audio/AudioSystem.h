#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

namespace Muk {

struct AudioListener {
    Vec3 Position{0,0,0};
    Vec3 Forward{0,0,1};
    Vec3 Up{0,1,0};
};

struct AudioSourceDesc {
    std::string ClipName;
    Vec3 Position{0,0,0};
    f32 Volume = 1.0f;
    f32 MinDistance = 1.0f;
    f32 MaxDistance = 40.0f;
    bool Loop = false;
    bool Spatial = true;
};

/**
 * Lightweight spatial audio.
 * Windows: XAudio2 + procedural beeps when no WAV loaded.
 * Clips can be registered as mono PCM or generated tones for prototyping.
 */
class AudioSystem {
public:
    bool Initialize();
    void Shutdown();

    void SetListener(const AudioListener& l) { m_Listener = l; }
    const AudioListener& GetListener() const { return m_Listener; }

    // Register silent/procedural clip by name (tone Hz, duration sec)
    bool RegisterTone(const std::string& name, f32 frequencyHz, f32 durationSec, f32 volume = 0.3f);

    // Play one-shot or looping spatial/non-spatial
    u32 Play(const AudioSourceDesc& desc);
    void Stop(u32 voiceId);
    void StopAll();

    void Update(f32 dt);

    bool IsReady() const { return m_Ready; }

private:
    struct Clip {
        std::vector<float> Samples; // mono float -1..1
        u32 SampleRate = 44100;
    };
    struct Voice {
        u32 Id = 0;
        std::string ClipName;
        Vec3 Position{};
        f32 Volume = 1;
        f32 MinD = 1, MaxD = 40;
        bool Loop = false;
        bool Spatial = true;
        f32 Cursor = 0; // sample index float
        bool Active = true;
    };

    f32 ComputeAttenuation(const Voice& v) const;

    std::unordered_map<std::string, Clip> m_Clips;
    std::vector<Voice> m_Voices;
    AudioListener m_Listener;
    u32 m_NextId = 1;
    bool m_Ready = false;

    // XAudio2 handles stored as void* to avoid header pollution in other TUs
    void* m_XAudio = nullptr;
    void* m_Mastering = nullptr;
};

} // namespace Muk
