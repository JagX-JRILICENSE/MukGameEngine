#pragma once

#include "AIClient.h"
#include "MultiAgentTeam.h"
#include "AsyncAI.h"
#include "Script/GameplayScript.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "Math/Vector.h"
#include <string>
#include <vector>

namespace Muk {

class Renderer;
class AudioSystem;
class ParticleSystem;
class ContentBrowser;
class UndoStack;
struct EditorEntityInfo;

enum class AgentPhase {
    Idle,
    Architecting,
    BuildingScene,
    WritingScript,
    BuildingUI,
    Previewing,
    Verifying,
    Fixing,
    Playtesting,
    Done,
    Failed
};

struct AgentLogLine {
    std::string Text;
    bool IsError = false;
};

class AIGameAgent {
public:
    void SetClient(AIClient* client);
    void SetSettings(const UserSettings& settings);
    void SetUndoStack(UndoStack* undo) { m_Undo = undo; }

    void StartBuild(const std::string& userBrief);
    void Cancel();

    void Tick(World& world, Renderer& renderer, AudioSystem* audio,
              std::vector<EditorEntityInfo>& entities, Entity& selected,
              ParticleSystem* particles = nullptr);

    GameRuntime& Runtime() { return m_Runtime; }
    MultiAgentTeam& Team() { return m_Team; }
    AsyncAI& Async() { return m_Async; }

    AgentPhase GetPhase() const { return m_Phase; }
    const std::string& GetStatus() const { return m_Status; }
    const std::vector<AgentLogLine>& GetLog() const { return m_Log; }
    bool IsBusy() const {
        return m_Phase != AgentPhase::Idle && m_Phase != AgentPhase::Done && m_Phase != AgentPhase::Failed;
    }

    void DrawImGui();
    bool SaveGeneratedAssets(ContentBrowser* browser = nullptr);

    void ApplyActions(World& world, Renderer& renderer, AudioSystem* audio,
                      std::vector<EditorEntityInfo>& entities, Entity& selected,
                      const std::string& text, ParticleSystem* particles = nullptr);

private:
    void Log(const std::string& s, bool err = false);
    void RunLocalVerify(World& world);
    ScriptHostCallbacks MakeHost(World& world, Renderer& renderer, AudioSystem* audio,
                                 std::vector<EditorEntityInfo>& entities, Entity& selected,
                                 ParticleSystem* particles);
    void SubmitPhase(AgentRole role, const std::string& tag,
                     const std::string& system, const std::string& user, float temp);
    void HandleAsyncResults(World& world, Renderer& renderer, AudioSystem* audio,
                            std::vector<EditorEntityInfo>& entities, Entity& selected,
                            ParticleSystem* particles);
    void FinishBatch(std::vector<EditorEntityInfo>* entities);

    std::string ExtractBlock(const std::string& text, const std::string& beginTag, const std::string& endTag) const;
    std::string ExtractScript(const std::string& text) const;

    MultiAgentTeam m_Team;
    AsyncAI m_Async;
    GameRuntime m_Runtime;
    UndoStack* m_Undo = nullptr;
    AgentPhase m_Phase = AgentPhase::Idle;
    bool m_WaitingAsync = false;
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
        "Build a complete mini-game: arena with floor, 4 pillars, 3 orbs. "
        "Player WASD. Collect orbs. Score 3 wins. HUD score.";

    bool m_UseDual = true;
};

} // namespace Muk
