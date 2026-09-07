#include <Muk/Engine.h>
#include <iostream>

using namespace Muk;

class SandboxApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Sandbox application initialized");
    }

    void OnUpdate(float deltaTime) override {
        // Game logic goes here
    }

    void OnRender() override {
        // Rendering will be handled by Renderer later
    }

    void OnShutdown() override {
        MUK_CORE_INFO("Sandbox application shutting down");
    }
};

int main() {
    SandboxApp app;
    app.Run();
    return 0;
}
