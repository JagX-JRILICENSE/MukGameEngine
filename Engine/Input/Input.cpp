#include "Input.h"

namespace Muk {

bool Input::s_Keys[512] = {};
bool Input::s_MouseButtons[8] = {};
Vec2 Input::s_MousePos{};

bool Input::IsKeyPressed(KeyCode key) {
    return s_Keys[static_cast<int>(key)];
}

bool Input::IsMouseButtonPressed(MouseButton button) {
    return s_MouseButtons[static_cast<int>(button)];
}

Vec2 Input::GetMousePosition() {
    return s_MousePos;
}

void Input::Update() {
    // Platform layer updates state; this can clear "just pressed" later
}

void Input::SetKeyState(KeyCode key, bool pressed) {
    s_Keys[static_cast<int>(key)] = pressed;
}

void Input::SetMouseButtonState(MouseButton button, bool pressed) {
    s_MouseButtons[static_cast<int>(button)] = pressed;
}

void Input::SetMousePosition(f32 x, f32 y) {
    s_MousePos = {x, y};
}

} // namespace Muk
