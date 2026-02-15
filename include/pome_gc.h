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
        T* object = nullptr; // Initialize to nullptr
        try {
            object = new T(std::forward<Args>(args)...);
        } catch (const std::bad_alloc& e) {
            std::cerr << "ERROR: std::bad_alloc caught during object allocation for type " << typeid(T).name() << ": " << e.what() << std::endl;
            throw; // Re-throw out of memory exception
        } catch (const std::exception& e) {
            std::cerr << "ERROR: Exception caught during object allocation for type " << typeid(T).name() << ": " << e.what() << std::endl;
            throw; // Re-throw other exceptions from constructor
        }

        if (object == nullptr) { // Defensive check, should theoretically not be hit with regular new
            std::cerr << "ERROR: 'new' returned nullptr unexpectedly for object of type " << typeid(T).name() << std::endl;
            return nullptr; 
        }

        object->gcSize = sizeof(T);
        object->generation = 0; // Young
        object->next = youngObjects_;
        youngObjects_ = object;
        bytesAllocated_ += sizeof(T); 
        
        addTemporaryRoot(object); // Protect from immediate collection during this allocate call
        
        if (bytesAllocated_ > nextGC_) {
            collect();
        }
        
        removeTemporaryRoot(object); // Remove temporary protection
        
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
    
    /**
     * Write Barrier: Call this when modifying an object's field/element
     */
    void writeBarrier(PomeObject* parent, PomeValue& child);

private:
    VM* vm_ = nullptr;
    
    PomeObject* youngObjects_ = nullptr;
    PomeObject* oldObjects_ = nullptr;
    
    std::vector<PomeObject*> rememberedSet_; // Old objects pointing to Young objects
    
    size_t bytesAllocated_ = 0;
    private:
        size_t nextGC_ = 1024 * 1024; // Trigger GC every 1MB initially

    std::vector<PomeObject*> tempRoots_;
    std::vector<PomeObject*> grayStack_; // Iterative marking stack

    void mark();
    void traceReferences(); // Process gray stack
    void markTable(std::map<PomeValue, PomeValue>& table);
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