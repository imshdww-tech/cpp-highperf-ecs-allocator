#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <cassert>
#include <cstddef>
#include <memory>
#include <unordered_set>
#include <utility>
#include <vector>

/**
 * @brief High-performance fixed-size Object Memory Pool.
 * Eliminates heap fragmentation and dynamic allocation overhead by pre-allocating
 * contiguous memory chunks.
 */
template <typename T, std::size_t BlockSize = 1024>
class MemoryPool {
private:
    static_assert(BlockSize > 0, "BlockSize must be greater than zero.");

    union Slot {
        T element;
        Slot* next;

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
        if (BlockSize > 0) {
            block[BlockSize - 1].next = freeSlots_;
        }

        freeSlots_ = &block[0];
        blocks_.push_back(std::move(block));
    }

    void release() noexcept {
        for (T* ptr : allocatedPointers_) {
            if (ptr != nullptr) {
                ptr->~T();
            }
        }

        allocatedPointers_.clear();
        freeSlots_ = nullptr;
        blocks_.clear();
    }

public:
    MemoryPool() noexcept = default;

    ~MemoryPool() noexcept {
        release();
    }

    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    MemoryPool(MemoryPool&& other) noexcept
        : blocks_(std::move(other.blocks_)),
          freeSlots_(other.freeSlots_),
          allocatedPointers_(std::move(other.allocatedPointers_)) {
        other.freeSlots_ = nullptr;
        other.allocatedPointers_.clear();
    }

    MemoryPool& operator=(MemoryPool&& other) noexcept {
        if (this != &other) {
            release();

            blocks_ = std::move(other.blocks_);
            freeSlots_ = other.freeSlots_;
            allocatedPointers_ = std::move(other.allocatedPointers_);

            other.freeSlots_ = nullptr;
            other.allocatedPointers_.clear();
        }
        return *this;
    }

    template <typename... Args>
    T* allocate(Args&&... args) {
        if (freeSlots_ == nullptr) {
            allocateBlock();
        }

        Slot* slot = freeSlots_;
        freeSlots_ = slot->next;

        T* result = reinterpret_cast<T*>(slot);
        new (result) T(std::forward<Args>(args)...);
        allocatedPointers_.insert(result);
        return result;
    }

    void deallocate(T* ptr) noexcept {
        if (ptr == nullptr) {
            return;
        }

        const auto it = allocatedPointers_.find(ptr);
        assert(it != allocatedPointers_.end() && "Deallocating pointer not owned by this MemoryPool.");
        if (it == allocatedPointers_.end()) {
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
