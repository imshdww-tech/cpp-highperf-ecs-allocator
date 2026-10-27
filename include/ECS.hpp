#ifndef ECS_HPP
#define ECS_HPP

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <vector>

using EntityID = std::uint32_t;
constexpr EntityID MAX_ENTITIES = 100000;

struct IComponentArray {
    virtual ~IComponentArray() = default;
    virtual void entityDestroyed(EntityID entity) = 0;
};

template <typename T>
class ComponentArray : public IComponentArray {
private:
    std::vector<T> data_;
    std::unordered_map<EntityID, std::size_t> entityToIndex_;
    std::unordered_map<std::size_t, EntityID> indexToEntity_;
    std::size_t size_ = 0;

public:
    bool hasData(EntityID entity) const {
        return entityToIndex_.find(entity) != entityToIndex_.end();
    }

    void insertData(EntityID entity, T value) {
        auto it = entityToIndex_.find(entity);
        if (it != entityToIndex_.end()) {
            data_[it->second] = value;
            return;
        }

        const std::size_t newIndex = size_;
        entityToIndex_[entity] = newIndex;
        indexToEntity_[newIndex] = entity;

        if (newIndex >= data_.size()) {
            data_.resize(newIndex + 1);
        }

        data_[newIndex] = value;
        ++size_;
    }

    void removeData(EntityID entity) {
        auto it = entityToIndex_.find(entity);
        if (it == entityToIndex_.end()) {
            return;
        }

        const std::size_t indexToRemove = it->second;
        const std::size_t lastIndex = size_ - 1;

        if (indexToRemove != lastIndex) {
            data_[indexToRemove] = data_[lastIndex];
            const EntityID movedEntity = indexToEntity_[lastIndex];
            entityToIndex_[movedEntity] = indexToRemove;
            indexToEntity_[indexToRemove] = movedEntity;
        }

        entityToIndex_.erase(entity);
        indexToEntity_.erase(lastIndex);
        --size_;

        if (data_.size() > size_) {
            data_.resize(size_);
        }
    }

    T& getData(EntityID entity) {
        const auto it = entityToIndex_.find(entity);
        if (it == entityToIndex_.end()) {
            throw std::out_of_range("Entity does not have this component.");
        }
        return data_[it->second];
    }

    const T& getData(EntityID entity) const {
        const auto it = entityToIndex_.find(entity);
        if (it == entityToIndex_.end()) {
            throw std::out_of_range("Entity does not have this component.");
        }
        return data_[it->second];
    }

    void entityDestroyed(EntityID entity) override {
        if (hasData(entity)) {
            removeData(entity);
        }
    }
};

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
        componentArrays_.try_emplace(typeName, std::make_shared<ComponentArray<T>>());
    }

    template <typename T>
    void addComponent(EntityID entity, T component) {
        if (entity >= MAX_ENTITIES || !entityAlive_[entity]) {
            throw std::out_of_range("Attempted to add component to an invalid or destroyed entity.");
        }

        getComponentArray<T>()->insertData(entity, std::move(component));
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
