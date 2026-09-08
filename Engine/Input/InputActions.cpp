#include "InputActions.h"

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>
#endif

namespace Muk {

void InputActions::Bind(const std::string& action, int vk) {
    m_Bindings[action] = vk;
}

void InputActions::BindDefaultGameplay() {
#ifdef MUK_PLATFORM_WINDOWS
    Bind("MoveForward", 'W');
    Bind("MoveBack", 'S');
    Bind("MoveLeft", 'A');
    Bind("MoveRight", 'D');
    Bind("Jump", VK_SPACE);
    Bind("ReloadScript", VK_F9);
    Bind("Screenshot", VK_F12);
    Bind("Play", VK_F5);
#endif
}

bool InputActions::IsDown(const std::string& action) const {
#ifdef MUK_PLATFORM_WINDOWS
    auto it = m_Bindings.find(action);
    if (it == m_Bindings.end()) return false;
    return (GetAsyncKeyState(it->second) & 0x8000) != 0;
#else
    (void)action;
    return false;
#endif
}

bool InputActions::WasPressed(const std::string& action) {
    bool now = IsDown(action);
    bool was = m_Prev[action];
    m_Prev[action] = now;
    return now && !was;
}

void InputActions::BeginFrame() {
    // edge state updated in WasPressed
}

} // namespace Muk
