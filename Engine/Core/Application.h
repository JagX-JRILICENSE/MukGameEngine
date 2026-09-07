#pragma once

#include "Core.h"
#include "Window.h"

namespace Muk {

class Application {
public:
    Application();
    virtual ~Application();

    void Run();
    void Close();

    Window& GetWindow() { return m_Window; }
    static Application& Get() { return *s_Instance; }

protected:
    virtual void OnInit() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnShutdown() {}

private:
    Window m_Window;
    bool m_Running = true;
    static Application* s_Instance;
};

} // namespace Muk
