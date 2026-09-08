#pragma once

#include "Entity.h"
#include "Component.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <typeindex>
#include <functional>

namespace Muk {

class World {
public:
    World() = default;
    ~World() = default;

    Entity CreateEntity();
    void DestroyEntity(Entity entity);

    template<typename T, typename... Args>
    T& AddComponent(Entity entity, Args&&... args) {
        auto& storage = m_Components[std::type_index(typeid(T))];
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = component.get();
        storage[entity.GetID()] = std::move(component);
        return *ptr;
    }

    template<typename T>
    T* GetComponent(Entity entity) {
        auto it = m_Components.find(std::type_index(typeid(T)));
        if (it == m_Components.end()) return nullptr;
        auto& storage = it->second;
        auto cit = storage.find(entity.GetID());
        if (cit == storage.end()) return nullptr;
        return static_cast<T*>(cit->second.get());
    }

    template<typename T>
    bool HasComponent(Entity entity) {
        return GetComponent<T>(entity) != nullptr;
    }

    template<typename T>
    void RemoveComponent(Entity entity) {
        auto it = m_Components.find(std::type_index(typeid(T)));
        if (it != m_Components.end())
            it->second.erase(entity.GetID());
    }

    template<typename T, typename Fn>
    void ForEach(Fn&& fn) {
        auto it = m_Components.find(std::type_index(typeid(T)));
        if (it == m_Components.end()) return;
        for (auto& [id, ptr] : it->second) {
            fn(Entity(id), *static_cast<T*>(ptr.get()));
        }
    }

    template<typename T1, typename T2, typename Fn>
    void ForEach(Fn&& fn) {
        auto it = m_Components.find(std::type_index(typeid(T1)));
        if (it == m_Components.end()) return;
        for (auto& [id, ptr] : it->second) {
            Entity e(id);
            T2* c2 = GetComponent<T2>(e);
            if (!c2) continue;
            fn(e, *static_cast<T1*>(ptr.get()), *c2);
        }
    }

    std::vector<Entity> GetEntitiesWithMesh() {
        std::vector<Entity> out;
        ForEach<MeshRenderer>([&](Entity e, MeshRenderer&) { out.push_back(e); });
        return out;
    }

private:
    EntityID m_NextEntityID = 1;
    std::vector<EntityID> m_FreeList;
    using ComponentMap = std::unordered_map<EntityID, std::unique_ptr<IComponent>>;
    std::unordered_map<std::type_index, ComponentMap> m_Components;
};

} // namespace Muk
