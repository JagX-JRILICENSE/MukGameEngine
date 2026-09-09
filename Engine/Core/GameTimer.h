#pragma once

#include "Core/Core.h"
#include <string>
#include <vector>
#include <functional>

namespace Muk {

struct TimedEvent {
    std::string Name;
    float TimeLeft = 0;
    float Duration = 0;
    bool Loop = false;
    bool Alive = true;
    std::function<void()> Callback;
};

class GameTimer {
public:
    void Schedule(const std::string& name, float seconds, std::function<void()> cb, bool loop = false) {
        TimedEvent e;
        e.Name = name;
        e.TimeLeft = seconds;
        e.Duration = seconds;
        e.Loop = loop;
        e.Callback = std::move(cb);
        m_Events.push_back(std::move(e));
    }

    void Cancel(const std::string& name) {
        for (auto& e : m_Events)
            if (e.Name == name) e.Alive = false;
    }

    void Update(float dt) {
        for (auto& e : m_Events) {
            if (!e.Alive) continue;
            e.TimeLeft -= dt;
            if (e.TimeLeft <= 0) {
                if (e.Callback) e.Callback();
                if (e.Loop) e.TimeLeft = e.Duration;
                else e.Alive = false;
            }
        }
        // compact
        std::vector<TimedEvent> live;
        for (auto& e : m_Events) if (e.Alive) live.push_back(std::move(e));
        m_Events.swap(live);
    }

    int Count() const { return (int)m_Events.size(); }

private:
    std::vector<TimedEvent> m_Events;
};

} // namespace Muk
