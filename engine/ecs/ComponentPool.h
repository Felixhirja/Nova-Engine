#pragma once
#include <memory>
#include <vector>
#include <cstddef>

namespace ecs {

// Memory pool for component allocations
template<typename T, size_t ChunkSize = 1024>
class ComponentPool {
public:
    ComponentPool() = default;
    ~ComponentPool() = default;
    
    T* Allocate() {
        if (freeList_.empty()) {
            AllocateChunk();
        }
        
        T* ptr = freeList_.back();
        freeList_.pop_back();
        return new (ptr) T();
    }
    
    template<typename... Args>
    T* Allocate(Args&&... args) {
        if (freeList_.empty()) {
            AllocateChunk();
        }
        
        T* ptr = freeList_.back();
        freeList_.pop_back();
        return new (ptr) T(std::forward<Args>(args)...);
    }
    
    void Deallocate(T* ptr) {
        if (ptr) {
            ptr->~T();
            freeList_.push_back(ptr);
        }
    }
    
    size_t GetChunkCount() const { return chunks_.size(); }
    size_t GetFreeCount() const { return freeList_.size(); }
    size_t GetTotalCapacity() const { return chunks_.size() * ChunkSize; }
    
private:
    struct Chunk {
        alignas(alignof(T)) unsigned char data[sizeof(T) * ChunkSize];
    };
    
    void AllocateChunk() {
        auto chunk = std::make_unique<Chunk>();
        T* base = reinterpret_cast<T*>(chunk->data);
        
        for (size_t i = 0; i < ChunkSize; ++i) {
            freeList_.push_back(base + i);
        }
        
        chunks_.push_back(std::move(chunk));
    }
    
    std::vector<std::unique_ptr<Chunk>> chunks_;
    std::vector<T*> freeList_;
};

} // namespace ecs
