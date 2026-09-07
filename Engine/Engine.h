#pragma once

/**
 * Muk Game Engine - Main public header
 * Include this for the full engine API.
 */

#include "Core/Core.h"
#include "Core/Application.h"
#include "Core/Log.h"
#include "Core/Window.h"
#include "Math/Math.h"
#include "ECS/World.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include "RHI/RHI.h"
#include "Renderer/Renderer.h"
#include "Input/Input.h"

namespace Muk {

class Engine {
public:
    struct Config {
        const char* windowTitle = "Muk Game Engine";
        int width = 1280;
        int height = 720;
        bool fullscreen = false;
        bool vsync = true;
    };

    Engine() = default;
    ~Engine();

    bool Initialize(const Config& config);
    void Run();
    void Shutdown();

    World& GetWorld() { return m_World; }
    Window& GetWindow() { return m_Window; }
    Renderer& GetRenderer() { return m_Renderer; }

private:
    bool m_Running = false;
    Window m_Window;
    World m_World;
    Renderer m_Renderer;
    Input m_Input;
};

} // namespace Muk
