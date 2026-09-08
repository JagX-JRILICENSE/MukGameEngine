#include "PlayInEditor.h"
#include "ECS/World.h"
#include "Physics/CharacterController.h"
#include "Core/Log.h"

namespace Muk {

void PlayInEditor::Play(World& world, CharacterController* character) {
    if (m_Playing) return;

    m_Snapshots.clear();
    world.ForEach<Transform>([&](Entity e, Transform& t) {
        PIETransformSnapshot s;
        s.Id = e.GetID();
        s.T = t;
        m_Snapshots.push_back(s);
    });

    m_HadChar = character && character->IsValid();
    if (m_HadChar)
        m_CharPos = character->GetPosition();

    m_Playing = true;
    MUK_CORE_INFO("PIE: Play ({0} transforms snapshotted)", (int)m_Snapshots.size());
}

void PlayInEditor::Stop(World& world, CharacterController* character) {
    if (!m_Playing) return;

    for (const auto& s : m_Snapshots) {
        Entity e(s.Id);
        if (auto* t = world.GetComponent<Transform>(e))
            *t = s.T;
    }

    if (m_HadChar && character && character->IsValid()) {
        // Re-sync visual; full teleport would need CharacterController API
        // Position is restored via transform; character re-created by editor if needed
        (void)m_CharPos;
    }

    m_Snapshots.clear();
    m_Playing = false;
    MUK_CORE_INFO("PIE: Stop — scene restored");
}

void PlayInEditor::Toggle(World& world, CharacterController* character) {
    if (m_Playing) Stop(world, character);
    else Play(world, character);
}

} // namespace Muk
