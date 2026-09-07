#include "World.h"
#include "Core/Log.h"

namespace Muk {

Entity World::CreateEntity() {
    EntityID id;
    if (!m_FreeList.empty()) {
        id = m_FreeList.back();
        m_FreeList.pop_back();
    } else {
        id = m_NextEntityID++;
    }
    MUK_CORE_TRACE("Created entity {0}", id);
    return Entity(id);
}

void World::DestroyEntity(Entity entity) {
    if (!entity.IsValid()) return;

    // Remove all components
    for (auto& [type, storage] : m_Components) {
        storage.erase(entity.GetID());
    }

    m_FreeList.push_back(entity.GetID());
    MUK_CORE_TRACE("Destroyed entity {0}", entity.GetID());
}

} // namespace Muk
