#pragma once

#include "AIClient.h"
#include "UserSettings.h"
#include <string>
#include <vector>

namespace Muk {

class Renderer;

/**
 * Editor panel: configure BYOK providers and chat with the model
 * to control / advise on the engine.
 */
class AIControlPanel {
public:
    void Initialize();
    void Draw(Renderer& renderer);

    UserSettings& Settings() { return m_Settings; }
    AIClient& Client() { return m_Client; }

private:
    void ApplySimpleActions(Renderer& renderer, const std::string& reply);

    UserSettings m_Settings;
    AIClient m_Client;
    char m_Input[2048] = {};
    char m_KeyEdit[512] = {};
    std::vector<std::string> m_Log;
    bool m_Busy = false;
};

} // namespace Muk
