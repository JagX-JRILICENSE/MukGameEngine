#pragma once

#include "Core/Core.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace Muk {

/** Named actions mapped to virtual keys (Windows VK codes as int) */
class InputActions {
public:
    void Bind(const std::string& action, int vk);
    void BindDefaultGameplay();

    bool IsDown(const std::string& action) const;
    bool WasPressed(const std::string& action); // edge detect

    void BeginFrame();

private:
    std::unordered_map<std::string, int> m_Bindings;
    std::unordered_map<std::string, bool> m_Prev;
};

} // namespace Muk
