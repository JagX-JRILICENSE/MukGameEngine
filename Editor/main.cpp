#include <Muk/Engine.h>
#include <iostream>

using namespace Muk;

/**
 * Muk Editor
 * Future: Full visual editor with viewport, outliner, details, content browser,
 * material editor, visual scripting, sequencer, etc.
 */
class EditorApp : public Application {
protected:
    void OnInit() override {
        MUK_CORE_INFO("Muk Editor started (placeholder)");
        MUK_CORE_INFO("Future features: Viewport | Outliner | Details | Content Browser | Visual Scripting");
    }

    void OnUpdate(float deltaTime) override {}
    void OnRender() override {}
    void OnShutdown() override {
        MUK_CORE_INFO("Muk Editor closed");
    }
};

int main() {
    EditorApp app;
    app.Run();
    return 0;
}
