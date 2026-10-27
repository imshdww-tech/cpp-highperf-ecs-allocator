#ifndef MEMORY_POOL_HPP
#define MEMORY_POOL_HPP

#include <cstddef>
#include <vector>
#include <utility>
#include <cassert>
#include <new>

/**
 * @brief High-performance fixed-size Object Memory Pool.
 * Eliminates heap fragmentation and dynamic allocation overhead by pre-allocating
 * contiguous memory chunks.
 */
template <typename T, std::size_t BlockSize = 1024>
class MemoryPool {
private:
    union Slot {
        T element;
        Slot* next;

        Slot() {}
        ~Slot() {}
    };

    Slot* currentBlock_ = nullptr;
    Slot* currentSlot_ = nullptr;
    Slot* freeSlots_ = nullptr;
    Slot* lastSlot_ = nullptr;

    void allocateBlock() {
        // Allocate contiguous memory block
        Slot* newBlock = reinterpret_cast<Slot*>(operator new(BlockSize * sizeof(Slot)));
        newBlock[0].next = currentBlock_;
        currentBlock_ = newBlock;

        // Set pointer boundaries for memory block
        currentSlot_ = newBlock + 1;
        lastSlot_ = newBlock + BlockSize;
    }

public:
    MemoryPool() noexcept = default;
    ~MemoryPool() noexcept {
        Slot* curr = currentBlock_;
        while (curr != nullptr) {
            Slot* next = curr[0].next;
            operator delete(reinterpret_cast<void*>(curr));
            curr = next;
        }
    }

    // Disable copying for safety
    MemoryPool(const MemoryPool&) = delete;
    MemoryPool& operator=(const MemoryPool&) = delete;

    // Allow Move Semantics
    MemoryPool(MemoryPool&& other) noexcept {
        currentBlock_ = other.currentBlock_;
        currentSlot_ = other.currentSlot_;
        freeSlots_ = other.freeSlots_;
        lastSlot_ = other.lastSlot_;
        other.currentBlock_ = nullptr;
    }

    template <typename... Args>
    T* allocate(Args&&... args) {
        if (freeSlots_ != nullptr) {
            T* result = reinterpret_cast<T*>(freeSlots_);
            freeSlots_ = freeSlots_->next;
            new (result) T(std::forward<Args>(args)...); // Placement NEW
            return result;
        }

        if (currentSlot_ >= lastSlot_) {
            allocateBlock();
        }

        T* result = reinterpret_cast<T*>(currentSlot_++);
        new (result) T(std::forward<Args>(args)...); // Placement NEW
        return result;
    }

    void deallocate(T* ptr) noexcept {
        if (ptr != nullptr) {
            ptr->~T(); // Explicit Destructor call
            Slot* slot = reinterpret_cast<Slot*>(ptr);
            slot->next = freeSlots_;
            freeSlots_ = slot;
        }
    }
};

#endif // MEMORY_POOL_HPP
