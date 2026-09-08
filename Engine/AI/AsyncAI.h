#pragma once

#include "AIClient.h"
#include "MultiAgentTeam.h"
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <functional>
#include <deque>

namespace Muk {

struct AsyncAIRequest {
    AgentRole Role = AgentRole::Architect;
    std::string System;
    std::string User;
    float Temperature = 0.35f;
    std::string Tag; // e.g. "architect", "builder"
};

struct AsyncAIResult {
    std::string Tag;
    AIResponse Response;
    bool Ready = false;
};

/**
 * Runs MultiAgentTeam::AskRole on a background thread.
 * Main thread: Submit() + Poll() — never blocks the editor frame.
 */
class AsyncAI {
public:
    AsyncAI() = default;
    ~AsyncAI() { Shutdown(); }

    void SetTeam(MultiAgentTeam* team) { m_Team = team; }

    void Shutdown();

    // Queue a request; returns job id
    u32 Submit(AsyncAIRequest req);

    // Drain finished results (call each frame)
    std::vector<AsyncAIResult> Poll();

    bool IsBusy() const { return m_Busy.load(); }
    int Pending() const;

private:
    void WorkerLoop();

    MultiAgentTeam* m_Team = nullptr;
    std::mutex m_Mutex;
    std::deque<std::pair<u32, AsyncAIRequest>> m_Queue;
    std::vector<AsyncAIResult> m_Done;
    std::thread m_Thread;
    std::atomic<bool> m_Running{ false };
    std::atomic<bool> m_Busy{ false };
    u32 m_NextId = 1;
};

} // namespace Muk
