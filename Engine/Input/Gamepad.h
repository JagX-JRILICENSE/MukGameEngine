#pragma once
#include "Core/Core.h"
#include <string>
#include <unordered_map>
#include <cmath>

#ifdef MUK_PLATFORM_WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <Xinput.h>
#endif

namespace Muk {

struct GamepadState {
    bool Connected = false;
    float LeftX = 0, LeftY = 0, RightX = 0, RightY = 0;
    float LT = 0, RT = 0;
    bool A = false, B = false, X = false, Y = false;
    bool Start = false, Back = false;
    bool LB = false, RB = false;
};

class Gamepad {
public:
    void Update(int index = 0) {
#ifdef MUK_PLATFORM_WINDOWS
        XINPUT_STATE xs{};
        if (XInputGetState((DWORD)index, &xs) == ERROR_SUCCESS) {
            m_State.Connected = true;
            auto dz = [](SHORT v) {
                float f = v / 32768.f;
                return (std::fabs(f) < 0.15f) ? 0.f : f;
            };
            m_State.LeftX = dz(xs.Gamepad.sThumbLX);
            m_State.LeftY = dz(xs.Gamepad.sThumbLY);
            m_State.RightX = dz(xs.Gamepad.sThumbRX);
            m_State.RightY = dz(xs.Gamepad.sThumbRY);
            m_State.LT = xs.Gamepad.bLeftTrigger / 255.f;
            m_State.RT = xs.Gamepad.bRightTrigger / 255.f;
            m_State.A = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0;
            m_State.B = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0;
            m_State.X = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0;
            m_State.Y = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0;
            m_State.Start = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0;
            m_State.LB = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0;
            m_State.RB = (xs.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0;
        } else {
            m_State = {};
        }
#else
        (void)index;
#endif
    }

    const GamepadState& State() const { return m_State; }

private:
    GamepadState m_State;
};

class InputRebind {
public:
    void SetDefault() {
#ifdef MUK_PLATFORM_WINDOWS
        m_Map["MoveForward"] = 'W';
        m_Map["MoveBack"] = 'S';
        m_Map["MoveLeft"] = 'A';
        m_Map["MoveRight"] = 'D';
        m_Map["Jump"] = VK_SPACE;
        m_Map["Interact"] = 'E';
        m_Map["Sprint"] = VK_SHIFT;
#endif
    }

    void Bind(const std::string& action, int vk) { m_Map[action] = vk; }
    int Get(const std::string& action, int def = 0) const {
        auto it = m_Map.find(action);
        return it == m_Map.end() ? def : it->second;
    }

    bool IsDown(const std::string& action) const {
#ifdef MUK_PLATFORM_WINDOWS
        int vk = Get(action);
        return vk && (GetAsyncKeyState(vk) & 0x8000) != 0;
#else
        (void)action; return false;
#endif
    }

    const std::unordered_map<std::string, int>& All() const { return m_Map; }

private:
    std::unordered_map<std::string, int> m_Map;
};

} // namespace Muk
