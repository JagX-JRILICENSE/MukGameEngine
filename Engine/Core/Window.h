#pragma once

#include "Core.h"
#include <string>
#include <functional>

struct GLFWwindow; // Forward declare if using GLFW later; currently Win32

namespace Muk {

struct WindowProps {
    std::string Title = "Muk Game Engine";
    u32 Width = 1280;
    u32 Height = 720;
    bool VSync = true;
    bool Fullscreen = false;
};

class Window {
public:
    using EventCallbackFn = std::function<void(void*)>; // Placeholder for event system

    Window() = default;
    ~Window();

    bool Create(const WindowProps& props);
    void Destroy();
    void PollEvents();
    void SwapBuffers();

    u32 GetWidth() const { return m_Data.Width; }
    u32 GetHeight() const { return m_Data.Height; }
    bool IsVSync() const { return m_Data.VSync; }
    void SetVSync(bool enabled);

    void* GetNativeHandle() const { return m_NativeHandle; }
    bool ShouldClose() const { return m_ShouldClose; }
    void SetShouldClose(bool value) { m_ShouldClose = value; }

private:
    struct WindowData {
        std::string Title;
        u32 Width = 0;
        u32 Height = 0;
        bool VSync = true;
    };

    WindowData m_Data;
    void* m_NativeHandle = nullptr; // HWND on Windows
    bool m_ShouldClose = false;
    bool m_Created = false;
};

} // namespace Muk
