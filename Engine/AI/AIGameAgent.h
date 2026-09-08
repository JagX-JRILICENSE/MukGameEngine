#pragma once

#include "AIClient.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "Math/Vector.h"
#include <string>
#include <vector>
#include <functional>

namespace Muk {

class Renderer;
class AudioSystem;
struct EditorEntityInfo;

enum class AgentPhase {
    Idle,
    Planning,
    Building,
    Previewing,
    Verifying,
    Fixing,
    Done,
    Failed
};

struct AgentLogLine {
    std::string Text;
    bool IsError = false;
};

/**
 * Autonomous game-builder loop:
 * Plan → emit ACTIONs → apply → preview checklist → verify → fix → repeat.
 * "Cursor" moves = select entity + focus camera on it.
 */
class AIGameAgent {
public:
    void SetClient(AIClient* client) { m_Client = client; }

    // Start a long-running design job from a natural language brief
    void StartBuild(const std::string& userBrief);
    void Cancel();

    // Call once per frame; may issue network calls when phase needs LLM
    void Tick(World& world, Renderer& renderer, AudioSystem* audio,
              std::vector<EditorEntityInfo>& entities, Entity& selected);

    AgentPhase GetPhase() const { return m_Phase; }
    const std::string& GetStatus() const { return m_Status; }
    const std::vector<AgentLogLine>& GetLog() const { return m_Log; }
    bool IsBusy() const {
        return m_Phase != AgentPhase::Idle && m_Phase != AgentPhase::Done && m_Phase != AgentPhase::Failed;
    }

    void DrawImGui();

private:
    void Log(const std::string& s, bool err = false);
    void ApplyActions(World& world, Renderer& renderer, AudioSystem* audio,
                      std::vector<EditorEntityInfo>& entities, Entity& selected,
                      const std::string& text);
    void RunVerify(World& world);
    std::string BuildPlanPrompt() const;
    std::string BuildVerifyPrompt(World& world) const;
    std::string BuildFixPrompt() const;

    AIClient* m_Client = nullptr;
    AgentPhase m_Phase = AgentPhase::Idle;
    std::string m_Brief;
    std::string m_Status;
    std::string m_LastPlan;
    std::string m_LastIssues;
    std::vector<AgentLogLine> m_Log;
    int m_FixAttempts = 0;
    static constexpr int kMaxFixAttempts = 4;

    char m_BriefEdit[1024] = "Make a small arena with a floor, 4 pillars, a player cube, and a warm light";
};

} // namespace Muk
