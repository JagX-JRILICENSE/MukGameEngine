#include "GameplayScript.h"
#include "Core/Log.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <cmath>

namespace Muk {

static std::string Trim(const std::string& s) {
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return {};
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

static std::vector<std::string> SplitWS(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string t;
    while (iss >> t) out.push_back(t);
    return out;
}

float GameplayScript::GetVar(const std::string& name, float def) const {
    auto it = m_Vars.find(name);
    return it == m_Vars.end() ? def : it->second;
}

void GameplayScript::Clear() {
    m_Source.clear();
    m_Lines.clear();
    m_Vars.clear();
    m_Errors.clear();
}

bool GameplayScript::LoadFromSource(const std::string& source) {
    Clear();
    m_Source = source;
    std::istringstream iss(source);
    std::string raw;
    Block cur = Block::None;
    std::string trigA, trigB;
    while (std::getline(iss, raw)) {
        // strip comments
        auto cmt = raw.find('#');
        if (cmt != std::string::npos) raw = raw.substr(0, cmt);
        int indent = 0;
        while (indent < (int)raw.size() && (raw[indent] == ' ' || raw[indent] == '\t')) ++indent;
        std::string line = Trim(raw);
        if (line.empty()) continue;

        std::string low = line;
        for (auto& ch : low) ch = (char)std::tolower((unsigned char)ch);

        if (low == "on_start") {
            cur = Block::OnStart; trigA.clear(); trigB.clear();
            continue;
        }
        if (low.rfind("on_update", 0) == 0) {
            cur = Block::OnUpdate; trigA.clear(); trigB.clear();
            continue;
        }
        if (low.rfind("on_trigger", 0) == 0) {
            auto tok = SplitWS(line);
            cur = Block::OnTrigger;
            trigA = tok.size() > 1 ? tok[1] : "";
            trigB = tok.size() > 2 ? tok[2] : "";
            continue;
        }

        Line L;
        L.BlockKind = cur;
        L.TriggerA = trigA;
        L.TriggerB = trigB;
        L.Raw = line;
        L.Indent = indent;
        m_Lines.push_back(L);
    }
    MUK_CORE_INFO("GameplayScript loaded: {0} lines", (int)m_Lines.size());
    return true;
}

float GameplayScript::EvalExpr(const std::string& expr, float dt) {
    std::string e = Trim(expr);
    if (e.empty()) return 0;
    // var
    if (m_Vars.count(e)) return m_Vars[e];
    if (e == "dt") return dt;
    // number
    try { return std::stof(e); } catch (...) {}
    // a*dt or a*b simple
    auto star = e.find('*');
    if (star != std::string::npos) {
        float a = EvalExpr(e.substr(0, star), dt);
        float b = EvalExpr(e.substr(star + 1), dt);
        return a * b;
    }
    auto plus = e.find('+');
    if (plus != std::string::npos) {
        return EvalExpr(e.substr(0, plus), dt) + EvalExpr(e.substr(plus + 1), dt);
    }
    auto minus = e.find('-');
    if (minus != std::string::npos && minus > 0) {
        return EvalExpr(e.substr(0, minus), dt) - EvalExpr(e.substr(minus + 1), dt);
    }
    return 0;
}

bool GameplayScript::EvalCondition(const std::string& cond, ScriptHostCallbacks& host, float dt) {
    auto c = Trim(cond);
    auto tok = SplitWS(c);
    if (tok.size() >= 2 && tok[0] == "key") {
        return host.IsKeyDown ? host.IsKeyDown(tok[1]) : false;
    }
    // score >= 3
    if (tok.size() >= 3) {
        float left = EvalExpr(tok[0], dt);
        float right = EvalExpr(tok[2], dt);
        if (tok[1] == ">=") return left >= right;
        if (tok[1] == "<=") return left <= right;
        if (tok[1] == ">") return left > right;
        if (tok[1] == "<") return left < right;
        if (tok[1] == "==") return std::fabs(left - right) < 1e-4f;
        if (tok[1] == "!=") return std::fabs(left - right) >= 1e-4f;
    }
    return EvalExpr(c, dt) != 0;
}

void GameplayScript::ExecLine(const std::string& raw, ScriptHostCallbacks& host, float dt) {
    auto line = Trim(raw);
    if (line.empty()) return;

    // if COND then REST
    if (line.rfind("if ", 0) == 0) {
        auto thenPos = line.find(" then ");
        if (thenPos == std::string::npos) return;
        std::string cond = line.substr(3, thenPos - 3);
        std::string rest = line.substr(thenPos + 6);
        if (EvalCondition(cond, host, dt))
            ExecLine(rest, host, dt);
        return;
    }

    auto tok = SplitWS(line);
    if (tok.empty()) return;
    const std::string& cmd = tok[0];

    if (cmd == "set" && tok.size() >= 3) {
        m_Vars[tok[1]] = EvalExpr(tok[2], dt);
    } else if (cmd == "add" && tok.size() >= 3) {
        m_Vars[tok[1]] = GetVar(tok[1]) + EvalExpr(tok[2], dt);
    } else if (cmd == "spawn_at" && tok.size() >= 5) {
        std::string name = tok[1];
        float x = EvalExpr(tok[2], dt), y = EvalExpr(tok[3], dt), z = EvalExpr(tok[4], dt);
        float sx = tok.size() > 5 ? EvalExpr(tok[5], dt) : 1;
        float sy = tok.size() > 6 ? EvalExpr(tok[6], dt) : 1;
        float sz = tok.size() > 7 ? EvalExpr(tok[7], dt) : 1;
        std::string mesh = tok.size() > 8 ? tok[8] : "Cube";
        if (host.Spawn) host.Spawn(name, x, y, z, sx, sy, sz, mesh);
    } else if (cmd == "destroy" && tok.size() >= 2) {
        if (host.DestroyByName) host.DestroyByName(tok[1]);
    } else if (cmd == "move" && tok.size() >= 5) {
        if (host.MoveByName)
            host.MoveByName(tok[1], EvalExpr(tok[2], dt), EvalExpr(tok[3], dt), EvalExpr(tok[4], dt));
    } else if (cmd == "load_level" && tok.size() >= 2) {
        if (host.LoadLevel) host.LoadLevel(tok[1]);
    } else if (cmd == "show_ui" && tok.size() >= 3) {
        std::string id = tok[1];
        std::string text;
        for (size_t i = 2; i < tok.size(); ++i) {
            if (i > 2) text += " ";
            text += tok[i];
        }
        // strip quotes
        if (!text.empty() && text.front() == '"') text.erase(0, 1);
        if (!text.empty() && text.back() == '"') text.pop_back();
        if (host.ShowUI) host.ShowUI(id, text);
    } else if (cmd == "hide_ui" && tok.size() >= 2) {
        if (host.HideUI) host.HideUI(tok[1]);
    } else if (cmd == "play_sound" && tok.size() >= 2) {
        if (host.PlaySound) host.PlaySound(tok[1]);
    } else if (cmd == "win") {
        std::string msg = tok.size() > 1 ? line.substr(line.find(tok[1])) : "Win";
        if (host.Win) host.Win(msg);
    } else if (cmd == "lose") {
        std::string msg = tok.size() > 1 ? line.substr(line.find(tok[1])) : "Lose";
        if (host.Lose) host.Lose(msg);
    }
}

void GameplayScript::CallOnStart(ScriptHostCallbacks& host) {
    for (auto& L : m_Lines)
        if (L.BlockKind == Block::OnStart)
            ExecLine(L.Raw, host, 0);
}

void GameplayScript::CallOnUpdate(ScriptHostCallbacks& host, float dt) {
    for (auto& L : m_Lines)
        if (L.BlockKind == Block::OnUpdate)
            ExecLine(L.Raw, host, dt);
}

void GameplayScript::CallOnTrigger(ScriptHostCallbacks& host, const std::string& a, const std::string& b) {
    for (auto& L : m_Lines) {
        if (L.BlockKind != Block::OnTrigger) continue;
        if ((L.TriggerA == a && L.TriggerB == b) || (L.TriggerA == b && L.TriggerB == a))
            ExecLine(L.Raw, host, 0);
    }
}

void GameRuntime::Clear() {
    m_Levels.clear();
    m_Script.Clear();
    m_UI.clear();
    m_Banner.clear();
    m_Playing = false;
    m_Won = m_Lost = false;
}

void GameRuntime::AddOrReplaceLevel(Level level) {
    for (auto& L : m_Levels) {
        if (L.Id == level.Id) { L = std::move(level); return; }
    }
    m_Levels.push_back(std::move(level));
}

bool GameRuntime::HasLevel(const std::string& id) const {
    for (auto& L : m_Levels) if (L.Id == id) return true;
    return false;
}

GameRuntime::Level* GameRuntime::GetLevel(const std::string& id) {
    for (auto& L : m_Levels) if (L.Id == id) return &L;
    return nullptr;
}

void GameRuntime::SetActiveScript(const std::string& source) {
    m_Script.LoadFromSource(source);
}

void GameRuntime::StartPlay(ScriptHostCallbacks host) {
    m_Host = std::move(host);
    // wrap UI/win into host
    auto userShow = m_Host.ShowUI;
    m_Host.ShowUI = [this, userShow](const std::string& id, const std::string& text) {
        bool found = false;
        for (auto& u : m_UI) if (u.Id == id) { u.Text = text; u.Visible = true; found = true; break; }
        if (!found) m_UI.push_back({ id, text, true });
        if (userShow) userShow(id, text);
    };
    auto userHide = m_Host.HideUI;
    m_Host.HideUI = [this, userHide](const std::string& id) {
        for (auto& u : m_UI) if (u.Id == id) u.Visible = false;
        if (userHide) userHide(id);
    };
    auto userWin = m_Host.Win;
    m_Host.Win = [this, userWin](const std::string& msg) {
        m_Won = true; m_Banner = msg;
        if (userWin) userWin(msg);
    };
    auto userLose = m_Host.Lose;
    m_Host.Lose = [this, userLose](const std::string& msg) {
        m_Lost = true; m_Banner = msg;
        if (userLose) userLose(msg);
    };

    m_Won = m_Lost = false;
    m_Banner.clear();
    m_Playing = true;
    m_Script.CallOnStart(m_Host);
}

void GameRuntime::StopPlay() {
    m_Playing = false;
}

void GameRuntime::Update(float dt) {
    if (!m_Playing || m_Won || m_Lost) return;
    m_Script.CallOnUpdate(m_Host, dt);
}

} // namespace Muk
