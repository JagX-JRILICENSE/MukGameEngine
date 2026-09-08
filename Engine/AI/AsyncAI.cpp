#include "AsyncAI.h"
#include "Core/Log.h"
#include <chrono>

namespace Muk {

void AsyncAI::Shutdown() {
    m_Running = false;
    if (m_Thread.joinable()) m_Thread.join();
}

u32 AsyncAI::Submit(AsyncAIRequest req) {
    u32 id = m_NextId++;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_Queue.push_back({ id, std::move(req) });
    }
    if (!m_Running.exchange(true)) {
        if (m_Thread.joinable()) m_Thread.join();
        m_Thread = std::thread([this]() { WorkerLoop(); });
    }
    return id;
}

int AsyncAI::Pending() const {
    // approximate
    return m_Busy.load() ? 1 : 0;
}

std::vector<AsyncAIResult> AsyncAI::Poll() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    std::vector<AsyncAIResult> out = std::move(m_Done);
    m_Done.clear();
    return out;
}

void AsyncAI::WorkerLoop() {
    while (true) {
        AsyncAIRequest req;
        std::string tag;
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            if (m_Queue.empty()) {
                m_Running = false;
                m_Busy = false;
                return;
            }
            auto item = std::move(m_Queue.front());
            m_Queue.pop_front();
            req = std::move(item.second);
            tag = req.Tag;
        }
        m_Busy = true;
        AIResponse resp;
        if (m_Team) {
            resp = m_Team->AskRole(req.Role, req.System, req.User, req.Temperature);
        } else {
            resp.Error = "No team";
        }
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            AsyncAIResult r;
            r.Tag = tag;
            r.Response = std::move(resp);
            r.Ready = true;
            m_Done.push_back(std::move(r));
        }
        m_Busy = false;
    }
}

} // namespace Muk
