#ifndef POME_GC_H
#define POME_GC_H

#include <vector>
#include <cstddef>
#include "pome_value.h"

namespace Pome {

class Interpreter; // Forward declaration

class GarbageCollector {
public:
    explicit GarbageCollector();
    
    /**
     * Set the interpreter instance for root marking
     */
    void setInterpreter(Interpreter* interpreter);

    template<typename T, typename... Args>
    T* allocate(Args&&... args) {
        T* object = new T(std::forward<Args>(args)...);
        object->gcSize = sizeof(T);
        object->next = head_;
        head_ = object;
        bytesAllocated_ += sizeof(T); 
        
        object->isMarked = true; // Protect from immediate collection
        
        if (bytesAllocated_ > nextGC_) {
            collect();
        }
        
        
        object->isMarked = false; // Reset to false for correct liveness tracking in future cycles
        
        return object;
    }

    void collect();
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

private:
    Interpreter* interpreter_ = nullptr;
    PomeObject* head_ = nullptr;
    size_t bytesAllocated_ = 0;
    size_t nextGC_ = 1024 * 1024; // Trigger GC every 1MB initially

    std::vector<PomeObject*> tempRoots_;

    void mark();
    void markTable(std::map<PomeValue, PomeValue>& table);
    void markEnvironmentStore(std::map<std::string, PomeValue>& store);
    void sweep();
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
