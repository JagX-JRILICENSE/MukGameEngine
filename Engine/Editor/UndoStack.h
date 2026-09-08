#pragma once

#include "Core/Core.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include <vector>
#include <string>
#include <memory>
#include <functional>

namespace Muk {

class World;

struct IUndoCommand {
    virtual ~IUndoCommand() = default;
    virtual void Undo(World& world) = 0;
    virtual void Redo(World& world) = 0;
    virtual const char* Name() const = 0;
};

struct TransformUndoCommand : public IUndoCommand {
    Entity Target;
    Transform Before;
    Transform After;
    std::string Label = "Transform";

    void Undo(World& world) override;
    void Redo(World& world) override;
    const char* Name() const override { return Label.c_str(); }
};

class UndoStack {
public:
    static constexpr size_t MaxDepth = 128;

    void Push(std::unique_ptr<IUndoCommand> cmd);
    void Undo(World& world);
    void Redo(World& world);
    void Clear();

    bool CanUndo() const { return m_Index > 0; }
    bool CanRedo() const { return m_Index < m_Stack.size(); }
    const char* PeekUndoName() const;
    const char* PeekRedoName() const;

    // Helper: record transform change after gizmo edit ends
    void BeginTransformEdit(Entity e, const Transform& before);
    void EndTransformEdit(Entity e, const Transform& after, World& world);

private:
    std::vector<std::unique_ptr<IUndoCommand>> m_Stack;
    size_t m_Index = 0; // next push index / redo pointer

    bool m_Editing = false;
    Entity m_EditEntity;
    Transform m_EditBefore;
};

} // namespace Muk
