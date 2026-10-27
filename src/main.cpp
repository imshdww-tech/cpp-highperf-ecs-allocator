#include <iostream>
#include <chrono>
#include "MemoryPool.hpp"
#include "ECS.hpp"

// Sample Components
struct Transform {
    float x, y, z;
};

struct Velocity {
    float vx, vy, vz;
};

int main() {
    std::cout << "========================================================\n";
    std::cout << " C++20 High-Performance ECS & Custom Memory Allocator\n";
    std::cout << "========================================================\n\n";

    // -------------------------------------------------------------
    // Benchmark 1: Custom Memory Pool vs Standard Dynamic Heap Allocation
    // -------------------------------------------------------------
    constexpr std::size_t ALLOC_COUNT = 500000;
    
    std::cout << "[1] Benchmarking Allocations (" << ALLOC_COUNT << " Objects)...\n";

    // Standard Heap Allocation Benchmark
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
    std::chrono::duration<double, std::milli> durationStandard = endStandard - startStandard;

    // Custom Memory Pool Benchmark
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
    std::chrono::duration<double, std::milli> durationPool = endPool - startPool;

    std::cout << " -> Standard Dynamic Heap (`new`/`delete`): " << durationStandard.count() << " ms\n";
    std::cout << " -> Custom Cache-Friendly MemoryPool:      " << durationPool.count() << " ms\n";
    std::cout << " -> Performance Gain: " << (durationStandard.count() / durationPool.count()) << "x Faster!\n\n";

    // -------------------------------------------------------------
    // Benchmark 2: Entity Component System Scene Update
    // -------------------------------------------------------------
    std::cout << "[2] Initializing ECS System & Registering Components...\n";
    Registry registry;
    registry.registerComponent<Transform>();
    registry.registerComponent<Velocity>();

    constexpr EntityID ENTITY_COUNT = 50000;
    std::vector<EntityID> entities;
    entities.reserve(ENTITY_COUNT);

    for (EntityID i = 0; i < ENTITY_COUNT; ++i) {
        EntityID e = registry.createEntity();
        registry.addComponent(e, Transform{0.0f, 0.0f, 0.0f});
        registry.addComponent(e, Velocity{1.5f, 0.5f, -0.2f});
        entities.push_back(e);
    }

    std::cout << " -> Simulated " << ENTITY_COUNT << " Entities updating movement vector...\n";

    auto startECS = std::chrono::high_resolution_clock::now();
    for (EntityID entity : entities) {
        auto& transform = registry.getComponent<Transform>(entity);
        auto& velocity = registry.getComponent<Velocity>(entity);

        transform.x += velocity.vx;
        transform.y += velocity.vy;
        transform.z += velocity.vz;
    }
    auto endECS = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> durationECS = endECS - startECS;

    std::cout << " -> Frame Process Time (50,000 Entities): " << durationECS.count() << " ms\n\n";
    std::cout << "Execution completed successfully.\n";

    return 0;
}
