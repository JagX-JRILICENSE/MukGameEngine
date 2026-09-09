#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <chrono>
#include <sstream>

namespace Muk {

class CrashLog {
public:
    static void Write(const std::string& msg) {
        std::ofstream out("muk_crash.log", std::ios::app);
        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        out << now << " | " << msg << "\n";
    }
};

class Analytics {
public:
    void Event(const std::string& name, const std::string& data = "") {
        m_Events.push_back(name + (data.empty() ? "" : ":" + data));
        if (m_Events.size() > 500) m_Events.erase(m_Events.begin(), m_Events.begin() + 100);
    }
    const std::vector<std::string>& Events() const { return m_Events; }
    bool Save(const std::string& path = "Assets/analytics.log") const {
        std::ofstream out(path);
        if (!out) return false;
        for (auto& e : m_Events) out << e << "\n";
        return true;
    }
private:
    std::vector<std::string> m_Events;
};

struct PackageManifest {
    std::string Name = "MukGame";
    std::string Version = "0.13.0";
    std::vector<std::string> Assets;
    std::vector<std::string> Scenes;

    bool Save(const std::string& path = "Assets/package.manifest") const {
        std::ofstream out(path);
        if (!out) return false;
        out << "name=" << Name << "\nversion=" << Version << "\n";
        for (auto& a : Assets) out << "asset=" << a << "\n";
        for (auto& s : Scenes) out << "scene=" << s << "\n";
        return true;
    }
};

/** Visual script graph (Blueprint-like nodes) — data model for AI + editor */
struct GraphNode {
    int Id = 0;
    std::string Type; // Event, Branch, Spawn, Move, Wait, AI
    std::string Title;
    float X = 0, Y = 0;
    std::vector<int> Outs;
};

class VisualGraph {
public:
    int AddNode(const std::string& type, const std::string& title, float x, float y) {
        GraphNode n;
        n.Id = m_NextId++;
        n.Type = type; n.Title = title; n.X = x; n.Y = y;
        m_Nodes.push_back(n);
        return n.Id;
    }
    void Link(int from, int to) {
        for (auto& n : m_Nodes) if (n.Id == from) n.Outs.push_back(to);
    }
    const std::vector<GraphNode>& Nodes() const { return m_Nodes; }

    void BuildDemoCollectibleGraph() {
        m_Nodes.clear(); m_NextId = 1;
        int start = AddNode("Event", "OnStart", 40, 40);
        int spawn = AddNode("Spawn", "Spawn Orbs", 200, 40);
        int loop = AddNode("Branch", "Score < 3?", 360, 40);
        int win = AddNode("Event", "Win", 520, 40);
        Link(start, spawn); Link(spawn, loop); Link(loop, win);
    }

private:
    std::vector<GraphNode> m_Nodes;
    int m_NextId = 1;
};

} // namespace Muk
