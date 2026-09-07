#pragma once

#include "Core/Core.h"
#include "Math/Vector.h"

namespace Muk {

enum class KeyCode {
    Unknown = 0,
    Space, A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    Escape, Enter, Tab, Backspace,
    Left, Right, Up, Down,
    // Add more as needed
};

enum class MouseButton {
    Left, Right, Middle
};

class Input {
public:
    static bool IsKeyPressed(KeyCode key);
    static bool IsMouseButtonPressed(MouseButton button);
    static Vec2 GetMousePosition();

    // Called by Window / platform layer
    static void Update();
    static void SetKeyState(KeyCode key, bool pressed);
    static void SetMouseButtonState(MouseButton button, bool pressed);
    static void SetMousePosition(f32 x, f32 y);

private:
    static bool s_Keys[512];
    static bool s_MouseButtons[8];
    static Vec2 s_MousePos;
};

} // namespace Muk
