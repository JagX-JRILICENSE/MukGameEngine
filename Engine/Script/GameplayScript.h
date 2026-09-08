#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <unordered_set>

namespace Muk {

struct ScriptHostCallbacks {
    std::function<void(const std::string& name, float x, float y, float z, float sx, float sy, float sz, const std::string& mesh)> Spawn;
    std::function<void(const std::string& name)> DestroyByName;
    std::function<void(const std::string& name, float x, float y, float z)> MoveByName;
    std::function<void(const std::string& levelId)> LoadLevel;
    std::function<void(const std::string& id, const std::string& text)> ShowUI;
    std::function<void(const std::string& id)> HideUI;
    std::function<void(const std::string& clip)> PlaySound;
    std::function<void(const std::string& msg)> Win;
    std::function<void(const std::string& msg)> Lose;
    std::function<bool(const std::string& key)> IsKeyDown;
    std::function<Vec3(const std::string& name)> GetPos;
};

class GameplayScript {
public:
    void Clear();
    bool LoadFromSource(const std::string& source);
    const std::string& Source() const { return m_Source; }

    void SetVar(const std::string& name, float v) { m_Vars[name] = v; }
    float GetVar(const std::string& name, float def = 0) const;

    void CallOnStart(ScriptHostCallbacks& host);
    void CallOnUpdate(ScriptHostCallbacks& host, float dt);
    void CallOnTrigger(ScriptHostCallbacks& host, const std::string& a, const std::string& b);

    // Distance triggers: when |A-B| < radius, fire once (orb pickup)
    void RegisterProximity(const std::string& a, const std::string& b, float radius);
    void UpdateProximity(ScriptHostCallbacks& host);

    bool HasErrors() const { return !m_Errors.empty(); }
    const std::vector<std::string>& Errors() const { return m_Errors; }

private:
    enum class Block { None, OnStart, OnUpdate, OnTrigger };

    struct Line {
        Block BlockKind = Block::None;
        std::string TriggerA, TriggerB;
        std::string Raw;
        int Indent = 0;
    };

    struct Proximity {
        std::string A, B;
        float Radius = 1.5f;
        bool Fired = false;
    };

    void ExecLine(const std::string& raw, ScriptHostCallbacks& host, float dt);
    bool EvalCondition(const std::string& cond, ScriptHostCallbacks& host, float dt);
    float EvalExpr(const std::string& expr, float dt);

    std::string m_Source;
    std::vector<Line> m_Lines;
    std::unordered_map<std::string, float> m_Vars;
    std::vector<std::string> m_Errors;
    std::vector<Proximity> m_Proximity;
};

class GameRuntime {
public:
    struct Level {
        std::string Id;
        std::string Name;
        std::string SceneActions;
        std::string ScriptSource;
    };

    struct UIElement {
        std::string Id;
        std::string Text;
        bool Visible = true;
    };

    void Clear();
    void AddOrReplaceLevel(Level level);
    bool HasLevel(const std::string& id) const;
    Level* GetLevel(const std::string& id);
    const std::vector<Level>& Levels() const { return m_Levels; }

    void SetActiveScript(const std::string& source);
    GameplayScript& Script() { return m_Script; }

    void StartPlay(ScriptHostCallbacks host);
    void StopPlay();
    void Update(float dt);
    bool IsPlaying() const { return m_Playing; }

    const std::vector<UIElement>& UI() const { return m_UI; }
    const std::string& Banner() const { return m_Banner; }
    bool HasWon() const { return m_Won; }
    bool HasLost() const { return m_Lost; }

    // Auto-save generated script under Assets/Scripts/
    static bool SaveScriptToAssets(const std::string& source, const std::string& filename);

    ScriptHostCallbacks& Host() { return m_Host; }

private:
    std::vector<Level> m_Levels;
    GameplayScript m_Script;
    ScriptHostCallbacks m_Host;
    std::vector<UIElement> m_UI;
    std::string m_Banner;
    bool m_Playing = false;
    bool m_Won = false;
    bool m_Lost = false;
};

} // namespace Muk
