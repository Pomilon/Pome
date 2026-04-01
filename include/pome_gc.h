#ifndef POME_GC_H
#define POME_GC_H

#include "pome_base.h"

namespace Pome {

class VM;

class GarbageCollector {
public:
    explicit GarbageCollector();
    
    void setVM(VM* vm);

    template<typename T, typename... Args>
    T* allocate(Args&&... args);

    void updateSize(PomeObject* obj, size_t oldSize, size_t newSize);

    void collect(bool minor = false);
    void addTemporaryRoot(PomeObject* obj);
    void removeTemporaryRoot(PomeObject* obj);

    void markObject(PomeObject* object);
    void markValue(PomeValue& value);

    size_t getObjectCount() const;
    size_t getGCCount() const { return gcCount_; }
    std::string getInfo() const;
    
    void writeBarrier(PomeObject* parent, PomeValue& child);

private:
    VM* vm_ = nullptr;
    size_t gcCount_ = 0;
    
    PomeObject* youngObjects_ = nullptr;
    PomeObject* oldObjects_ = nullptr;
    
    std::vector<PomeObject*> rememberedSet_; 
    
    size_t bytesAllocated_ = 0;
    size_t youngBytesAllocated_ = 0;
    size_t nextGC_ = 16 * 1024 * 1024; 
    size_t nextMinorGC_ = 4 * 1024 * 1024; 

    std::vector<PomeObject*> tempRoots_;
    std::vector<PomeObject*> grayStack_; 

    void mark(bool minor);
    void traceReferences(bool minor); 
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
    
    RootGuard(const RootGuard&) = delete;
    RootGuard& operator=(const RootGuard&) = delete;

private:
    GarbageCollector& gc_;
    PomeObject* obj_;
};

} // namespace Pome

#include "pome_gc_impl.h"

#endif // POME_GC_H
