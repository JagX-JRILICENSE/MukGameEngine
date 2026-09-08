#pragma once

#include "Core.h"
#include "Window.h"
#include "Renderer/Renderer.h"
#include "Physics/PhysicsWorld.h"
#include "Asset/AssetManager.h"
#include "ECS/World.h"

namespace Muk {

class Application {
public:
    Application();
    virtual ~Application();

    void Run();
    void Close();

    Window& GetWindow() { return m_Window; }
    Renderer& GetRenderer() { return m_Renderer; }
    PhysicsWorld& GetPhysics() { return m_Physics; }
    AssetManager& GetAssets() { return m_Assets; }
    World& GetWorld() { return m_World; }

    static Application& Get() { return *s_Instance; }

protected:
    virtual void OnInit() {}
    virtual void OnUpdate(float deltaTime) {}
    virtual void OnRender() {}
    virtual void OnShutdown() {}

    // Convenience accessors for derived classes
    Window& Window() { return m_Window; }
    Renderer& Renderer() { return m_Renderer; }
    PhysicsWorld& Physics() { return m_Physics; }
    AssetManager& Assets() { return m_Assets; }
    World& ECS() { return m_World; }

private:
    Window m_Window;
    Renderer m_Renderer;
    PhysicsWorld m_Physics;
    AssetManager m_Assets;
    World m_World;

    bool m_Running = true;
    static Application* s_Instance;
};

} // namespace Muk
