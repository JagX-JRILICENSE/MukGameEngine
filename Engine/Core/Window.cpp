#include "Window.h"
#include "Log.h"

#ifdef MUK_PLATFORM_WINDOWS
#include <Windows.h>

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            // Handle resize later
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif

namespace Muk {

Window::~Window() {
    Destroy();
}

bool Window::Create(const WindowProps& props) {
#ifdef MUK_PLATFORM_WINDOWS
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;
    m_Data.VSync = props.VSync;

    HINSTANCE hInstance = GetModuleHandle(nullptr);

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"MukWindowClass";

    if (!RegisterClassEx(&wc)) {
        MUK_CORE_ERROR("Failed to register window class");
        return false;
    }

    RECT rect = { 0, 0, (LONG)props.Width, (LONG)props.Height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowEx(
        0,
        L"MukWindowClass",
        std::wstring(props.Title.begin(), props.Title.end()).c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hwnd) {
        MUK_CORE_ERROR("Failed to create window");
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    m_NativeHandle = hwnd;
    m_Created = true;
    m_ShouldClose = false;

    MUK_CORE_INFO("Window created: {0}x{1} - {2}", props.Width, props.Height, props.Title.c_str());
    return true;
#else
    MUK_CORE_ERROR("Window creation not implemented for this platform");
    return false;
#endif
}

void Window::Destroy() {
#ifdef MUK_PLATFORM_WINDOWS
    if (m_NativeHandle) {
        DestroyWindow((HWND)m_NativeHandle);
        m_NativeHandle = nullptr;
    }
    UnregisterClass(L"MukWindowClass", GetModuleHandle(nullptr));
#endif
    m_Created = false;
}

void Window::PollEvents() {
#ifdef MUK_PLATFORM_WINDOWS
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_ShouldClose = true;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif
}

void Window::SwapBuffers() {
    // Will be handled by RHI / DXGI swapchain later
}

void Window::SetVSync(bool enabled) {
    m_Data.VSync = enabled;
    // Implement with DXGI later
}

} // namespace Muk
