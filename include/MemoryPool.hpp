#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <cassert>
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * @brief Fixed-size Object Memory Pool.
 *
 * This pool pre-allocates blocks of memory and reuses them for objects of a
 * single type. It avoids heap fragmentation and keeps allocation/deallocation
 * fast for short-lived, repeated object lifecycles.
 */
template <typename T, std::size_t BlockSize = 1024>
class MemoryPool {
private:
    static_assert(BlockSize > 0, "BlockSize must be greater than zero.");

    struct Slot {
        union {
            T element;
            Slot* next;
        };

        Slot() : next(nullptr) {}
        ~Slot() = default;
    };

    std::vector<std::unique_ptr<Slot[]>> blocks_;
    Slot* freeSlots_ = nullptr;
    std::unordered_set<T*> allocatedPointers_;

    void allocateBlock() {
        auto block = std::make_unique<Slot[]>(BlockSize);

        for (std::size_t i = 0; i + 1 < BlockSize; ++i) {
            block[i].next = &block[i + 1];
        }
        block[BlockSize - 1].next = freeSlots_;

        freeSlots_ = &block[0];
        blocks_.push_back(std::move(block));
    }

    void destroyAllLiveObjects() noexcept {
        for (T* ptr : allocatedPointers_) {
            if (ptr != nullptr) {
                ptr->~T();
            }
        }
        allocatedPointers_.clear();
    }

public:
    MemoryPool() noexcept = default;

    ~MemoryPool() noexcept {
        destroyAllLiveObjects();
        freeSlots_ = nullptr;
        blocks_.clear();
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(MemoryPool&& other) noexcept = default;
    MemoryPool& operator=(MemoryPool&& other) noexcept = default;

    template <typename... Args>
    T* allocate(Args&&... args) {
        if (freeSlots_ == nullptr) {
            allocateBlock();
        }

        Slot* slot = freeSlots_;
        freeSlots_ = slot->next;

        T* ptr = std::launder(reinterpret_cast<T*>(slot));
        new (ptr) T(std::forward<Args>(args)...);
        allocatedPointers_.insert(ptr);
        return ptr;
    }

    void deallocate(T* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        const auto it = allocatedPointers_.find(ptr);
        if (it == allocatedPointers_.end()) {
            assert(false && "Pointer not owned by this memory pool.");
            return;
        }

        ptr->~T();
        allocatedPointers_.erase(it);

        Slot* slot = reinterpret_cast<Slot*>(ptr);
        slot->next = freeSlots_;
        freeSlots_ = slot;
    }
};

#endif // MEMORY_POOL_HPP
