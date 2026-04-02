#ifndef POME_GC_H
#define POME_GC_H

#include "pome_base.h"

#include <fstream>
#include <sstream>

namespace Pome {

class VM;

class GarbageCollector {
public:
    static size_t getRSS() {
        std::ifstream is("/proc/self/status");
        std::string line;
        while (std::getline(is, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::stringstream ss(line.substr(7));
                size_t rss;
                ss >> rss;
                return rss; // in KB
            }
        }
        return 0;
    }

    explicit GarbageCollector();
    
    void setVM(VM* vm);

    template<typename T, typename... Args>
    T* allocate(Args&&... args);
    PomeList* allocateList();

    void updateSize(PomeObject* obj, size_t oldSize, size_t newSize);

    void collect(bool minor = false);
    void addTemporaryRoot(PomeObject* obj);
    void removeTemporaryRoot(PomeObject* obj);

    void markObject(PomeObject* object);
    void markValue(const PomeValue& value);

    size_t getObjectCount() const;
    size_t getGCCount() const { return gcCount_; }
    std::string getInfo() const;
    PomeShape* getRootShape() const;
    void dumpHeap() const;
    
    void writeBarrier(PomeObject* parent, const PomeValue& child);

    bool pendingGC = false;
    bool shouldCollectMinor() const { return youngBytesAllocated_ > nextMinorGC_; }

private:
    VM* vm_ = nullptr;
    size_t gcCount_ = 0;
    
    PomeObject* youngObjects_ = nullptr;
    PomeObject* oldObjects_ = nullptr;
    
    std::vector<PomeObject*> rememberedSet_; 
    std::vector<PomeObject*> listPool_;
    
    size_t bytesAllocated_ = 0;
    size_t youngBytesAllocated_ = 0;
    size_t nextGC_ = 64 * 1024 * 1024; 
    size_t nextMinorGC_ = 16 * 1024 * 1024; 

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
