#include "Application.h"
#include "Log.h"
#include "Profiler.h"

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
        return;
    }

    if (!m_Renderer.Initialize(m_Window.GetNativeHandle(), props.Width, props.Height)) {
        MUK_CORE_ERROR("Failed to initialize Renderer - continuing with limited functionality");
    }

    m_Physics.Initialize();
    m_Assets.Initialize();
}

Application::~Application() {
    m_Assets.Shutdown();
    m_Physics.Shutdown();
    m_Renderer.Shutdown();
    m_Window.Destroy();
    s_Instance = nullptr;
}

void Application::Run() {
    OnInit();
    MUK_CORE_INFO("Muk Game Engine started");

    while (m_Running && !m_Window.ShouldClose()) {
        Profiler::Get().BeginFrame();

        m_Window.PollEvents();
        float deltaTime = 1.0f / 60.0f;

        {
            MUK_PROFILE_SCOPE("Physics");
            m_Physics.Update(deltaTime);
        }
        {
            MUK_PROFILE_SCOPE("Update");
            OnUpdate(deltaTime);
        }

        m_Renderer.BeginFrame();
        {
            MUK_PROFILE_SCOPE("Render");
            OnRender();
        }
        m_Renderer.EndFrame();
        m_Window.SwapBuffers();

        Profiler::Get().EndFrame();
    }

    OnShutdown();
    MUK_CORE_INFO("Muk Game Engine shut down");
}

void Application::Close() {
    m_Running = false;
    m_Window.SetShouldClose(true);
}

} // namespace Muk
