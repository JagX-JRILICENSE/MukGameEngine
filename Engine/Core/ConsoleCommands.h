#pragma once

#include "Core/Core.h"
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <sstream>

namespace Muk {

class ConsoleCommands {
public:
    using Handler = std::function<std::string(const std::vector<std::string>& args)>;

    void Register(const std::string& name, Handler h, const std::string& help = "") {
        m_Handlers[name] = std::move(h);
        m_Help[name] = help;
    }

    std::string Execute(const std::string& line) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;
        if (cmd.empty()) return {};
        std::vector<std::string> args;
        std::string a;
        while (iss >> a) args.push_back(a);
        if (cmd == "help") {
            std::string out = "Commands:\n";
            for (auto& [n, h] : m_Help) out += "  " + n + " — " + h + "\n";
            return out;
        }
        auto it = m_Handlers.find(cmd);
        if (it == m_Handlers.end()) return "Unknown command: " + cmd + " (try help)";
        return it->second(args);
    }

    void DrawImGui(char* inputBuf, size_t inputSize, std::vector<std::string>& history) {
#ifdef MUK_USE_IMGUI
        // implemented in editor to avoid imgui include here — optional
        (void)inputBuf; (void)inputSize; (void)history;
#endif
    }

private:
    std::unordered_map<std::string, Handler> m_Handlers;
    std::unordered_map<std::string, std::string> m_Help;
};

} // namespace Muk
