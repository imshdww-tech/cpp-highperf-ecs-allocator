#ifndef ECS_HPP
#define ECS_HPP

#include <vector>
#include <unordered_map>
#include <memory>
#include <typeindex>
#include <iostream>
#include <cstdint>

using EntityID = std::uint32_t;
constexpr EntityID MAX_ENTITIES = 100000;

// Abstract Component Interface
struct IComponentArray {
    virtual ~IComponentArray() = default;
    virtual void entityDestroyed(EntityID entity) = 0;
};

// Packed Component Array for Sequential CPU Cache Access
template <typename T>
class ComponentArray : public IComponentArray {
private:
    std::vector<T> componentArray_;
    std::unordered_map<EntityID, std::size_t> entityToIndexMap_;
    std::unordered_map<std::size_t, EntityID> indexToEntityMap_;
    std::size_t size_ = 0;

public:
    void insertData(EntityID entity, T component) {
        std::size_t newIndex = size_;
        entityToIndexMap_[entity] = newIndex;
        indexToEntityMap_[newIndex] = entity;
        
        if (newIndex < componentArray_.size()) {
            componentArray_[newIndex] = component;
        } else {
            componentArray_.push_back(component);
        }
        size_++;
    }

    void removeData(EntityID entity) {
        if (entityToIndexMap_.find(entity) == entityToIndexMap_.end()) return;

        // Keep array contiguous by swapping removed element with the last element
        std::size_t indexOfRemovedEntity = entityToIndexMap_[entity];
        std::size_t indexOfLastElement = size_ - 1;
        componentArray_[indexOfRemovedEntity] = componentArray_[indexOfLastElement];

        EntityID entityOfLastElement = indexToEntityMap_[indexOfLastElement];
        entityToIndexMap_[entityOfLastElement] = indexOfRemovedEntity;
        indexToEntityMap_[indexOfRemovedEntity] = entityOfLastElement;

        entityToIndexMap_.erase(entity);
        indexToEntityMap_.erase(indexOfLastElement);
        size_--;
    }

    T& getData(EntityID entity) {
        return componentArray_[entityToIndexMap_.at(entity)];
    }

    void entityDestroyed(EntityID entity) override {
        if (entityToIndexMap_.find(entity) != entityToIndexMap_.end()) {
            removeData(entity);
        }
    }

    std::vector<T>& getRawArray() { return componentArray_; }
    std::size_t getSize() const { return size_; }
};

// Coordinator for Entities and Components
class Registry {
private:
    EntityID livingEntityCount_ = 0;
    std::vector<EntityID> availableEntities_;
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays_;

    template <typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray() {
        std::type_index typeName = std::type_index(typeid(T));
        return std::static_pointer_cast<ComponentArray<T>>(componentArrays_[typeName]);
    }

public:
    Registry() {
        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
            availableEntities_.push_back(entity);
        }
    }

    EntityID createEntity() {
        EntityID id = availableEntities_.back();
        availableEntities_.pop_back();
        livingEntityCount_++;
        return id;
    }

    void destroyEntity(EntityID entity) {
        for (auto const& pair : componentArrays_) {
            pair.second->entityDestroyed(entity);
        }
        availableEntities_.push_back(entity);
        livingEntityCount_--;
    }

    template <typename T>
    void registerComponent() {
        std::type_index typeName = std::type_index(typeid(T));
        componentArrays_[typeName] = std::make_shared<ComponentArray<T>>();
    }

    template <typename T>
    void addComponent(EntityID entity, T component) {
        getComponentArray<T>()->insertData(entity, component);
    }

    template <typename T>
    T& getComponent(EntityID entity) {
        return getComponentArray<T>()->getData(entity);
    }
};

#endif // ECS_HPP
