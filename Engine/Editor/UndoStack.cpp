#include "UndoStack.h"
#include "ECS/World.h"
#include "EditorUI/EditorUI.h"
#include "Core/Log.h"
#include <cmath>

namespace Muk {

static bool TransformsEqual(const Transform& a, const Transform& b) {
    auto nearf = [](f32 x, f32 y) { return std::fabs(x - y) < 1e-4f; };
    return nearf(a.Position.x, b.Position.x) && nearf(a.Position.y, b.Position.y) && nearf(a.Position.z, b.Position.z)
        && nearf(a.Rotation.x, b.Rotation.x) && nearf(a.Rotation.y, b.Rotation.y) && nearf(a.Rotation.z, b.Rotation.z)
        && nearf(a.Scale.x, b.Scale.x) && nearf(a.Scale.y, b.Scale.y) && nearf(a.Scale.z, b.Scale.z);
}

void TransformUndoCommand::Undo(World& world) {
    if (auto* t = world.GetComponent<Transform>(Target))
        *t = Before;
}

void TransformUndoCommand::Redo(World& world) {
    if (auto* t = world.GetComponent<Transform>(Target))
        *t = After;
}

void AISpawnBatchCommand::Undo(World& world) {
    for (auto e : Spawned) {
        if (e.IsValid())
            world.DestroyEntity(e);
    }
    if (TrackList) {
        for (const auto& name : Names) {
            for (size_t i = 0; i < TrackList->size(); ++i) {
                if ((*TrackList)[i].Name == name) {
                    TrackList->erase(TrackList->begin() + (std::ptrdiff_t)i);
                    break;
                }
            }
        }
    }
    MUK_CORE_INFO("AI spawn batch undone ({0} entities)", (int)Spawned.size());
}

void AISpawnBatchCommand::Redo(World&) {
    // Spawns are not recreated automatically; user re-runs AI build
    MUK_CORE_INFO("AI spawn redo: re-run AI build to recreate");
}

void UndoStack::Push(std::unique_ptr<IUndoCommand> cmd) {
    if (m_Index < m_Stack.size())
        m_Stack.erase(m_Stack.begin() + static_cast<std::ptrdiff_t>(m_Index), m_Stack.end());
    m_Stack.push_back(std::move(cmd));
    if (m_Stack.size() > MaxDepth)
        m_Stack.erase(m_Stack.begin());
    m_Index = m_Stack.size();
}

void UndoStack::Undo(World& world) {
    if (!CanUndo()) return;
    --m_Index;
    m_Stack[m_Index]->Undo(world);
    MUK_CORE_INFO("Undo: {0}", m_Stack[m_Index]->Name());
}

void UndoStack::Redo(World& world) {
    if (!CanRedo()) return;
    m_Stack[m_Index]->Redo(world);
    MUK_CORE_INFO("Redo: {0}", m_Stack[m_Index]->Name());
    ++m_Index;
}

void UndoStack::Clear() {
    m_Stack.clear();
    m_Index = 0;
    m_Editing = false;
    m_AIBatchOpen = false;
    m_AIEntities.clear();
    m_AINames.clear();
}

const char* UndoStack::PeekUndoName() const {
    return CanUndo() ? m_Stack[m_Index - 1]->Name() : "";
}

const char* UndoStack::PeekRedoName() const {
    return CanRedo() ? m_Stack[m_Index]->Name() : "";
}

void UndoStack::BeginTransformEdit(Entity e, const Transform& before) {
    if (m_Editing) return;
    m_Editing = true;
    m_EditEntity = e;
    m_EditBefore = before;
}

void UndoStack::EndTransformEdit(Entity e, const Transform& after, World&) {
    if (!m_Editing || e.GetID() != m_EditEntity.GetID()) {
        m_Editing = false;
        return;
    }
    m_Editing = false;
    if (TransformsEqual(m_EditBefore, after)) return;

    auto cmd = std::make_unique<TransformUndoCommand>();
    cmd->Target = e;
    cmd->Before = m_EditBefore;
    cmd->After = after;
    cmd->Label = "Transform";
    Push(std::move(cmd));
}

void UndoStack::BeginAISpawnBatch() {
    m_AIBatchOpen = true;
    m_AIEntities.clear();
    m_AINames.clear();
}

void UndoStack::RecordAISpawn(Entity e, const std::string& name) {
    if (!m_AIBatchOpen) return;
    m_AIEntities.push_back(e);
    m_AINames.push_back(name);
}

void UndoStack::EndAISpawnBatch(std::vector<EditorEntityInfo>* trackList) {
    if (!m_AIBatchOpen) return;
    m_AIBatchOpen = false;
    if (m_AIEntities.empty()) return;
    auto cmd = std::make_unique<AISpawnBatchCommand>();
    cmd->Spawned = m_AIEntities;
    cmd->Names = m_AINames;
    cmd->TrackList = trackList;
    Push(std::move(cmd));
    m_AIEntities.clear();
    m_AINames.clear();
    MUK_CORE_INFO("AI spawn batch recorded for undo");
}

} // namespace Muk
