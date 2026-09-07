#include "Application.h"
#include "Log.h"

namespace Muk {

Application* Application::s_Instance = nullptr;

Application::Application() {
    s_Instance = this;
    Log::Init();

    WindowProps props;
    props.Title = "Muk Game Engine";
    props.Width = 1280;
    props.Height = 720;

    if (!m_Window.Create(props)) {
        MUK_CORE_CRITICAL("Failed to create window!");
        m_Running = false;
    }
}

Application::~Application() {
    m_Window.Destroy();
    s_Instance = nullptr;
}

void Application::Run() {
    OnInit();

    MUK_CORE_INFO("Muk Game Engine started");

    while (m_Running && !m_Window.ShouldClose()) {
        m_Window.PollEvents();

        // Fixed timestep placeholder
        float deltaTime = 1.0f / 60.0f;

        OnUpdate(deltaTime);
        OnRender();

        m_Window.SwapBuffers();
    }

    OnShutdown();
    MUK_CORE_INFO("Muk Game Engine shut down");
}

void Application::Close() {
    m_Running = false;
    m_Window.SetShouldClose(true);
}

} // namespace Muk
