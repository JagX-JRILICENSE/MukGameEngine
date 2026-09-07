#pragma once

#include "Core/Core.h"

namespace Muk {

using EntityID = u32;
constexpr EntityID INVALID_ENTITY = 0;

class Entity {
public:
    Entity() = default;
    explicit Entity(EntityID id) : m_ID(id) {}

    EntityID GetID() const { return m_ID; }
    bool IsValid() const { return m_ID != INVALID_ENTITY; }

    bool operator==(const Entity& other) const { return m_ID == other.m_ID; }
    bool operator!=(const Entity& other) const { return m_ID != other.m_ID; }

private:
    EntityID m_ID = INVALID_ENTITY;
};

} // namespace Muk
