#ifndef POME_GC_H
#define POME_GC_H

#include <vector>
#include <cstddef>
#include "pome_value.h"
#include <typeinfo> // For typeid

namespace Pome {

class VM; // Forward declaration

class GarbageCollector {
public:
    explicit GarbageCollector();
    
    /**
     * Set the VM instance for root marking
     */
    void setVM(VM* vm);

    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        // 1. Check if we need to collect BEFORE allocating
        if (youngBytesAllocated_ > nextMinorGC_) {
            collect(true); // Minor GC
        }
        if (bytesAllocated_ > nextGC_) {
            collect(false); // Major GC
        }

        T* object = nullptr;
        try {
            object = new T(std::forward<Args>(args)...);
        } catch (const std::bad_alloc& e) {
            // Force Major GC and try one last time
            collect(false);
            object = new T(std::forward<Args>(args)...);
        }

        object->gcSize = sizeof(T);
        object->generation = 0; // Young
        object->age = 0;
        object->next = youngObjects_;
        youngObjects_ = object;
        
        if constexpr (std::is_same_v<T, PomeString>) {
            object->gcSize += ((PomeString*)object)->extraSize();
        } else if constexpr (std::is_same_v<T, PomeList>) {
            object->gcSize += ((PomeList*)object)->extraSize();
        }
        
        bytesAllocated_ += object->gcSize; 
        youngBytesAllocated_ += object->gcSize;
        
        return object;
    }

    /**
     * Update an object's GC size when it grows (e.g. string or list append)
     */
    void updateSize(PomeObject* obj, size_t oldSize, size_t newSize) {
        if (newSize > oldSize) {
            size_t diff = newSize - oldSize;
            bytesAllocated_ += diff;
            if (obj->generation == 0) youngBytesAllocated_ += diff;
            obj->gcSize = newSize;
        } else if (oldSize > newSize) {
            size_t diff = oldSize - newSize;
            bytesAllocated_ -= diff;
            if (obj->generation == 0) youngBytesAllocated_ -= diff;
            obj->gcSize = newSize;
        }
    }

    void collect(bool minor = false);
    void addTemporaryRoot(PomeObject* obj);
    void removeTemporaryRoot(PomeObject* obj);

    /**
     * Public marking for Interpreter
     */
    void markObject(PomeObject* object);
    void markValue(PomeValue& value);

    /**
     * Debugging
     */
    size_t getObjectCount() const;
    size_t getGCCount() const { return gcCount_; }
    std::string getInfo() const;
    
    /**
     * Write Barrier: Call this when modifying an object's field/element
     */
    void writeBarrier(PomeObject* parent, PomeValue& child);

private:
    VM* vm_ = nullptr;
    size_t gcCount_ = 0;
    
    PomeObject* youngObjects_ = nullptr;
    PomeObject* oldObjects_ = nullptr;
    
    std::vector<PomeObject*> rememberedSet_; // Old objects pointing to Young objects
    
    size_t bytesAllocated_ = 0;
    size_t youngBytesAllocated_ = 0;
    private:
        size_t nextGC_ = 16 * 1024 * 1024; // Trigger Major GC when total heap > 16MB
        size_t nextMinorGC_ = 4 * 1024 * 1024; // Trigger Minor GC when young gen > 4MB

    std::vector<PomeObject*> tempRoots_;
    std::vector<PomeObject*> grayStack_; // Iterative marking stack

    void mark(bool minor);
    void traceReferences(bool minor); // Process gray stack
    void markTable(std::map<PomeValue, PomeValue>& table);
    void sweep(bool minor);
};

class RootGuard {
public:
    RootGuard(GarbageCollector& gc, PomeObject* obj) : gc_(gc), obj_(obj) {
        if (obj_) gc_.addTemporaryRoot(obj_);
    }
    ~RootGuard() {
        if (obj_) gc_.removeTemporaryRoot(obj_);
    }
    
    /**
     * Disable copy
     */
    RootGuard(const RootGuard&) = delete;
    RootGuard& operator=(const RootGuard&) = delete;

private:
    GarbageCollector& gc_;
    PomeObject* obj_;
};

} // namespace Pome

#endif // POME_GC_H
