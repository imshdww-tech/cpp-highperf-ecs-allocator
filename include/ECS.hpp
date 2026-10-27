#ifndef ECS_HPP
#define ECS_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

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
    bool hasData(EntityID entity) const {
        return entityToIndexMap_.find(entity) != entityToIndexMap_.end();
    }

    void insertData(EntityID entity, T component) {
        auto it = entityToIndexMap_.find(entity);
        if (it != entityToIndexMap_.end()) {
            componentArray_[it->second] = component;
            return;
        }

        const std::size_t newIndex = size_;
        entityToIndexMap_[entity] = newIndex;
        indexToEntityMap_[newIndex] = entity;

        if (newIndex >= componentArray_.size()) {
            componentArray_.resize(newIndex + 1);
        }

        componentArray_[newIndex] = component;
        ++size_;
    }

    void removeData(EntityID entity) {
        auto it = entityToIndexMap_.find(entity);
        if (it == entityToIndexMap_.end()) {
            return;
        }

        const std::size_t indexOfRemovedEntity = it->second;
        const std::size_t indexOfLastElement = size_ - 1;

        if (indexOfRemovedEntity != indexOfLastElement) {
            componentArray_[indexOfRemovedEntity] = componentArray_[indexOfLastElement];

            const EntityID entityOfLastElement = indexToEntityMap_[indexOfLastElement];
            entityToIndexMap_[entityOfLastElement] = indexOfRemovedEntity;
            indexToEntityMap_[indexOfRemovedEntity] = entityOfLastElement;
        }

        entityToIndexMap_.erase(entity);
        indexToEntityMap_.erase(indexOfLastElement);

        if (size_ > 0) {
            --size_;
        }

        if (componentArray_.size() > size_) {
            componentArray_.resize(size_);
        }
    }

    T& getData(EntityID entity) {
        const auto it = entityToIndexMap_.find(entity);
        if (it == entityToIndexMap_.end()) {
            throw std::out_of_range("Entity does not have this component.");
        }
        return componentArray_[it->second];
    }

    const T& getData(EntityID entity) const {
        const auto it = entityToIndexMap_.find(entity);
        if (it == entityToIndexMap_.end()) {
            throw std::out_of_range("Entity does not have this component.");
        }
        return componentArray_[it->second];
    }

    void entityDestroyed(EntityID entity) override {
        if (hasData(entity)) {
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
    std::vector<bool> entityAlive_;
    std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> componentArrays_;

    template <typename T>
    std::shared_ptr<ComponentArray<T>> getComponentArray() {
        const std::type_index typeName = std::type_index(typeid(T));
        const auto it = componentArrays_.find(typeName);
        if (it == componentArrays_.end()) {
            throw std::runtime_error("Component type is not registered.");
        }
        return std::static_pointer_cast<ComponentArray<T>>(it->second);
    }

public:
    Registry() : entityAlive_(MAX_ENTITIES, false) {
        availableEntities_.reserve(MAX_ENTITIES);
        for (EntityID entity = 0; entity < MAX_ENTITIES; ++entity) {
            availableEntities_.push_back(entity);
        }
    }

    EntityID createEntity() {
        if (availableEntities_.empty()) {
            throw std::runtime_error("No available entities to create.");
        }

        const EntityID id = availableEntities_.back();
        availableEntities_.pop_back();
        entityAlive_[id] = true;
        ++livingEntityCount_;
        return id;
    }

    void destroyEntity(EntityID entity) {
        if (entity >= MAX_ENTITIES || !entityAlive_[entity]) {
            return;
        }

        for (auto const& pair : componentArrays_) {
            pair.second->entityDestroyed(entity);
        }

        entityAlive_[entity] = false;
        availableEntities_.push_back(entity);
        --livingEntityCount_;
    }

    template <typename T>
    void registerComponent() {
        const std::type_index typeName = std::type_index(typeid(T));
        if (componentArrays_.find(typeName) == componentArrays_.end()) {
            componentArrays_[typeName] = std::make_shared<ComponentArray<T>>();
        }
    }

    template <typename T>
    void addComponent(EntityID entity, T component) {
        if (entity >= MAX_ENTITIES || !entityAlive_[entity]) {
            throw std::out_of_range("Attempted to add component to an invalid or destroyed entity.");
        }

        getComponentArray<T>()->insertData(entity, component);
    }

    template <typename T>
    T& getComponent(EntityID entity) {
        if (entity >= MAX_ENTITIES || !entityAlive_[entity]) {
            throw std::out_of_range("Attempted to access a component on an invalid or destroyed entity.");
        }

        return getComponentArray<T>()->getData(entity);
    }

    EntityID getLivingEntityCount() const { return livingEntityCount_; }
};

#endif // ECS_HPP
