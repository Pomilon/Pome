#ifndef POME_GC_IMPL_H
#define POME_GC_IMPL_H

namespace Pome {

    template<typename T, typename... Args>
    T* GarbageCollector::allocate(Args&&... args) {
        if (youngBytesAllocated_ > nextMinorGC_) {
            collect(true);
        }
        if (bytesAllocated_ > nextGC_) {
            collect(false);
        }

        T* object = nullptr;
        try {
            object = new T(std::forward<Args>(args)...);
        } catch (const std::bad_alloc& e) {
            collect(false);
            object = new T(std::forward<Args>(args)...);
        }

        PomeObject* obj = static_cast<PomeObject*>(object);
        obj->gcSize = sizeof(T) + obj->extraSize();

        obj->generation = 0;
        obj->age = 0;
        obj->refCount = 0;
        obj->inZCT = true;
        zct_.push_back(obj);

        obj->next = youngObjects_;
        youngObjects_ = obj;
        
        bytesAllocated_ += obj->gcSize; 
        youngBytesAllocated_ += obj->gcSize;
        
        return object;
    }

}

#endif // POME_GC_IMPL_H
