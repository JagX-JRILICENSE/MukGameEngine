#pragma once

#include "AIClient.h"
#include "MultiAgentTeam.h"
#include "Script/GameplayScript.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "Math/Vector.h"
#include <string>
#include <vector>

namespace Muk {

class Renderer;
class AudioSystem;
struct EditorEntityInfo;

enum class AgentPhase {
    Idle,
    Architecting,   // multi-AI design doc + levels
    BuildingScene,  // ACTION spawn/layout
    WritingScript,  // gameplay script
    BuildingUI,     // HUD / flow actions
    Previewing,
    Verifying,
    Fixing,
    Playtesting,    // run script briefly / host play
    Done,
    Failed
};

struct AgentLogLine {
    std::string Text;
    bool IsError = false;
};

/**
 * Full production-style AI pipeline:
 * Architect + Builder + Scripter + Critic (OpenRouter and/or NVIDIA)
 * produces levels, scene ACTIONs, Muk Script gameplay, UI flows,
 * then preview / verify / fix / playtest.
 */
class AIGameAgent {
public:
    void SetClient(AIClient* client);
    void SetSettings(const UserSettings& settings);

    void StartBuild(const std::string& userBrief);
    void Cancel();

    void Tick(World& world, Renderer& renderer, AudioSystem* audio,
              std::vector<EditorEntityInfo>& entities, Entity& selected);

    GameRuntime& Runtime() { return m_Runtime; }
    MultiAgentTeam& Team() { return m_Team; }

    AgentPhase GetPhase() const { return m_Phase; }
    const std::string& GetStatus() const { return m_Status; }
    const std::vector<AgentLogLine>& GetLog() const { return m_Log; }
    bool IsBusy() const {
        return m_Phase != AgentPhase::Idle && m_Phase != AgentPhase::Done && m_Phase != AgentPhase::Failed;
    }

    void DrawImGui();

    // Apply ACTION block (used by runtime load_level too)
    void ApplyActions(World& world, Renderer& renderer, AudioSystem* audio,
                      std::vector<EditorEntityInfo>& entities, Entity& selected,
                      const std::string& text);

private:
    void Log(const std::string& s, bool err = false);
    void RunLocalVerify(World& world);
    ScriptHostCallbacks MakeHost(World& world, Renderer& renderer, AudioSystem* audio,
                                 std::vector<EditorEntityInfo>& entities, Entity& selected);

    std::string ExtractBlock(const std::string& text, const std::string& beginTag, const std::string& endTag) const;
    std::string ExtractScript(const std::string& text) const;

    MultiAgentTeam m_Team;
    GameRuntime m_Runtime;
    AgentPhase m_Phase = AgentPhase::Idle;
    std::string m_Brief;
    std::string m_Status;
    std::string m_DesignDoc;
    std::string m_SceneActions;
    std::string m_ScriptSource;
    std::string m_UIActions;
    std::string m_LastIssues;
    std::vector<AgentLogLine> m_Log;
    int m_FixAttempts = 0;
    static constexpr int kMaxFixAttempts = 6;
    float m_PlayTestTimer = 0;

    char m_BriefEdit[2048] =
        "Build a complete mini-game: 2 levels. Level1 arena with floor, 4 pillars, 3 orbs. "
        "Player moves with WASD. Collect orbs (score). At score 3 load level2 boss platform and win. "
        "Show HUD score. Include title UI.";

    bool m_UseDual = true;
};

} // namespace Muk
