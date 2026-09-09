#pragma once
#include "Core/Core.h"
#include "Math/Vector.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <cmath>
#include <algorithm>

namespace Muk {

struct HealthComponent {
    float Max = 100.f;
    float Current = 100.f;
    bool Invulnerable = false;
    void Damage(float d) { if (!Invulnerable) Current = std::max(0.f, Current - d); }
    void Heal(float h) { Current = std::min(Max, Current + h); }
    bool IsDead() const { return Current <= 0.f; }
};

struct InventorySlot { std::string ItemId; int Count = 0; };
class Inventory {
public:
    void Add(const std::string& id, int n = 1) {
        for (auto& s : m_Slots) if (s.ItemId == id) { s.Count += n; return; }
        m_Slots.push_back({ id, n });
    }
    bool Remove(const std::string& id, int n = 1) {
        for (auto& s : m_Slots) if (s.ItemId == id && s.Count >= n) { s.Count -= n; return true; }
        return false;
    }
    int Count(const std::string& id) const {
        for (auto& s : m_Slots) if (s.ItemId == id) return s.Count;
        return 0;
    }
    const std::vector<InventorySlot>&Slots() const { return m_Slots; }
private:
    std::vector<InventorySlot> m_Slots;
};

struct Quest {
    std::string Id, Title;
    int Progress = 0, Goal = 1;
    bool Complete = false;
    void AddProgress(int n = 1) { Progress = std::min(Goal, Progress + n); Complete = Progress >= Goal; }
};
class QuestLog {
public:
    void Start(const Quest& q) { m_Quests[q.Id] = q; }
    void Progress(const std::string& id, int n = 1) {
        if (m_Quests.count(id)) m_Quests[id].AddProgress(n);
    }
    const std::unordered_map<std::string, Quest>& All() const { return m_Quests; }
private:
    std::unordered_map<std::string, Quest> m_Quests;
};

struct DialogueLine { std::string Speaker, Text; std::vector<std::string> Choices; };
class DialogueSystem {
public:
    void SetLines(std::vector<DialogueLine> lines) { m_Lines = std::move(lines); m_Index = 0; m_Active = true; }
    bool Active() const { return m_Active; }
    const DialogueLine* Current() const {
        return (m_Active && m_Index < (int)m_Lines.size()) ? &m_Lines[m_Index] : nullptr;
    }
    void Advance(int choice = 0) {
        (void)choice; ++m_Index;
        if (m_Index >= (int)m_Lines.size()) m_Active = false;
    }
private:
    std::vector<DialogueLine> m_Lines;
    int m_Index = 0;
    bool m_Active = false;
};

struct SaveSlot {
    int Index = 0;
    std::string Name;
    float PlayTime = 0;
    std::string Scene;
};
class SaveSystem {
public:
    void SetSlot(int i, const SaveSlot& s) { if (i >= 0 && i < 3) m_Slots[i] = s; }
    SaveSlot& Slot(int i) { return m_Slots[std::clamp(i, 0, 2)]; }
private:
    SaveSlot m_Slots[3];
};

struct WeatherSystem {
    enum class Type { Clear, Rain, Storm, Fog } Current = Type::Clear;
    float Intensity = 0.f;
    void Set(Type t, float intensity = 1.f) { Current = t; Intensity = intensity; }
    void Update(float dt) {
        if (Current == Type::Rain || Current == Type::Storm)
            Intensity = std::min(1.f, Intensity + dt * 0.1f);
    }
};

struct MinimapMarker { Vec3 Pos; std::string Label; int Type = 0; };
class Minimap {
public:
    void Clear() { m_Markers.clear(); }
    void Add(const MinimapMarker& m) { m_Markers.push_back(m); }
    const std::vector<MinimapMarker>& Markers() const { return m_Markers; }
private:
    std::vector<MinimapMarker> m_Markers;
};

class Achievements {
public:
    void Unlock(const std::string& id) { m_Unlocked.insert(id); }
    bool Has(const std::string& id) const { return m_Unlocked.count(id) > 0; }
    int Count() const { return (int)m_Unlocked.size(); }
private:
    std::unordered_set<std::string> m_Unlocked;
};

struct ComboMeter {
    int Count = 0;
    float Timer = 0;
    float Window = 1.5f;
    void Hit() { ++Count; Timer = Window; }
    void Update(float dt) { if (Timer > 0) { Timer -= dt; if (Timer <= 0) Count = 0; } }
};

struct VehicleState {
    Vec3 Position{};
    float Speed = 0;
    float MaxSpeed = 20.f;
    float Steer = 0;
    void Update(float throttle, float steer, float dt) {
        Steer = steer;
        Speed = std::clamp(Speed + throttle * 10.f * dt, -MaxSpeed * 0.3f, MaxSpeed);
        float yaw = Steer * Speed * 0.05f;
        Position.x += std::sin(yaw) * Speed * dt;
        Position.z += std::cos(yaw) * Speed * dt;
        Speed *= (1.f - 0.5f * dt);
    }
};

class BuildGrid {
public:
    void Configure(int w, int d) { m_W = w; m_D = d; m_Occ.assign(w * d, 0); }
    bool CanPlace(int x, int z) const {
        return x >= 0 && z >= 0 && x < m_W && z < m_D && m_Occ[z * m_W + x] == 0;
    }
    bool Place(int x, int z, int id) {
        if (!CanPlace(x, z)) return false;
        m_Occ[z * m_W + x] = id; return true;
    }
private:
    int m_W = 0, m_D = 0;
    std::vector<int> m_Occ;
};

struct Wallet {
    int Coins = 0;
    void Earn(int n) { Coins += n; }
    bool Spend(int n) { if (Coins < n) return false; Coins -= n; return true; }
};

struct StealthState {
    float Detection = 0;
    void Update(bool inLight, bool moving, float dt) {
        float target = (inLight ? 0.6f : 0.1f) + (moving ? 0.3f : 0.f);
        Detection = std::clamp(Detection + (target - Detection) * dt * 2.f, 0.f, 1.f);
    }
    bool Spotted() const { return Detection > 0.85f; }
};

class WaveSpawner {
public:
    void Configure(int waves, int perWave) { m_Waves = waves; m_Per = perWave; m_Current = 0; }
    bool StartNext(std::function<void(int index)> spawnFn) {
        if (m_Current >= m_Waves) return false;
        for (int i = 0; i < m_Per; ++i) spawnFn(i);
        ++m_Current;
        return true;
    }
    int CurrentWave() const { return m_Current; }
private:
    int m_Waves = 3, m_Per = 5, m_Current = 0;
};

} // namespace Muk
