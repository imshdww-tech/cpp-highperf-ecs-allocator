#include <chrono>
#include <iostream>
#include <vector>

#include "MemoryPool.hpp"
#include "ECS.hpp"

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Velocity {
    float vx = 0.0f;
    float vy = 0.0f;
    float vz = 0.0f;
};

int main() {
    std::cout << "========================================================\n";
    std::cout << " C++20 High-Performance ECS & Custom Memory Allocator\n";
    std::cout << "========================================================\n\n";

    constexpr std::size_t ALLOC_COUNT = 500000;
    std::cout << "[1] Benchmarking allocations (" << ALLOC_COUNT << " objects)...\n";

    auto startStandard = std::chrono::high_resolution_clock::now();
    std::vector<Transform*> standardPointers;
    standardPointers.reserve(ALLOC_COUNT);
    for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
        standardPointers.push_back(new Transform{1.0f, 2.0f, 3.0f});
    }
    for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
        delete standardPointers[i];
    }
    auto endStandard = std::chrono::high_resolution_clock::now();
    const auto durationStandard = std::chrono::duration<double, std::milli>(endStandard - startStandard);

    auto startPool = std::chrono::high_resolution_clock::now();
    MemoryPool<Transform, 4096> pool;
    std::vector<Transform*> poolPointers;
    poolPointers.reserve(ALLOC_COUNT);
    for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
        poolPointers.push_back(pool.allocate(1.0f, 2.0f, 3.0f));
    }
    for (std::size_t i = 0; i < ALLOC_COUNT; ++i) {
        pool.deallocate(poolPointers[i]);
    }
    auto endPool = std::chrono::high_resolution_clock::now();
    const auto durationPool = std::chrono::duration<double, std::milli>(endPool - startPool);

    std::cout << " -> Standard dynamic heap (`new`/`delete`): " << durationStandard.count() << " ms\n";
    std::cout << " -> Custom memory pool: " << durationPool.count() << " ms\n";

    const double poolTimeMs = durationPool.count();
    const double standardTimeMs = durationStandard.count();
    const double performanceGain = (poolTimeMs > 0.0) ? (standardTimeMs / poolTimeMs) : 0.0;
    std::cout << " -> Performance gain: " << performanceGain << "x\n\n";

    std::cout << "[2] Initializing ECS and simulating entities...\n";
    Registry registry;
    registry.registerComponent<Transform>();
    registry.registerComponent<Velocity>();

    constexpr EntityID ENTITY_COUNT = 50000;
    std::vector<EntityID> entities;
    entities.reserve(ENTITY_COUNT);

    for (EntityID i = 0; i < ENTITY_COUNT; ++i) {
        const EntityID entity = registry.createEntity();
        registry.addComponent(entity, Transform{0.0f, 0.0f, 0.0f});
        registry.addComponent(entity, Velocity{1.5f, 0.5f, -0.2f});
        entities.push_back(entity);
    }

    auto startECS = std::chrono::high_resolution_clock::now();
    for (EntityID entity : entities) {
        auto& transform = registry.getComponent<Transform>(entity);
        auto& velocity = registry.getComponent<Velocity>(entity);

        transform.x += velocity.vx;
        transform.y += velocity.vy;
        transform.z += velocity.vz;
    }
    auto endECS = std::chrono::high_resolution_clock::now();
    const auto durationECS = std::chrono::duration<double, std::milli>(endECS - startECS);

    std::cout << " -> Frame process time (50,000 entities): " << durationECS.count() << " ms\n\n";
    std::cout << "Execution completed successfully.\n";
    return 0;
}
