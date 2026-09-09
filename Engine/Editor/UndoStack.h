#pragma once

#include "Core/Core.h"
#include "ECS/Entity.h"
#include "ECS/Component.h"
#include <vector>
#include <string>
#include <memory>

namespace Muk {

class World;
struct EditorEntityInfo;

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

/** Snapshot of entities created by AI — Undo destroys them, Redo is no-op (rebuild via AI) */
struct AISpawnBatchCommand : public IUndoCommand {
    std::vector<Entity> Spawned;
    std::vector<std::string> Names;
    std::vector<EditorEntityInfo>* TrackList = nullptr; // optional hierarchy list

    void Undo(World& world) override;
    void Redo(World& world) override;
    const char* Name() const override { return "AI Spawn Batch"; }
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

    void BeginTransformEdit(Entity e, const Transform& before);
    void EndTransformEdit(Entity e, const Transform& after, World& world);

    // AI batch helpers
    void BeginAISpawnBatch();
    void RecordAISpawn(Entity e, const std::string& name);
    void EndAISpawnBatch(std::vector<EditorEntityInfo>* trackList);
    bool IsAIBatchOpen() const { return m_AIBatchOpen; }

private:
    std::vector<std::unique_ptr<IUndoCommand>> m_Stack;
    size_t m_Index = 0;

    bool m_Editing = false;
    Entity m_EditEntity;
    Transform m_EditBefore;

    bool m_AIBatchOpen = false;
    std::vector<Entity> m_AIEntities;
    std::vector<std::string> m_AINames;
};

} // namespace Muk
