#include "../include/pome_gc.h"
#include "../include/pome_vm.h"
#include <iostream>
#include <sstream>

namespace Pome {

GarbageCollector::GarbageCollector() {}

void GarbageCollector::setVM(VM* vm) {
    vm_ = vm;
}

void GarbageCollector::updateSize(PomeObject* obj, size_t oldSize, size_t newSize) {
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

void GarbageCollector::collect(bool minor) {

    gcCount_++;
    mark(minor);
    sweep(minor);
}

void GarbageCollector::writeBarrier(PomeObject* parent, PomeValue& child) {
    if (parent && parent->generation == 1 && child.isObject()) {
        PomeObject* childObj = child.asObject();
        if (childObj && childObj->generation == 0) {
            rememberedSet_.push_back(parent);
        }
    }
}

void GarbageCollector::mark(bool minor) {
    if (vm_) {
        vm_->markRoots();
    }
    
    for (auto* obj : tempRoots_) {
        markObject(obj);
    }
    
    if (minor) {
        for (auto* obj : rememberedSet_) {
            markObject(obj);
        }
    }
    
    traceReferences(minor);
}

void GarbageCollector::traceReferences(bool minor) {
    while (!grayStack_.empty()) {
        PomeObject* object = grayStack_.back();
        grayStack_.pop_back();
        object->markChildren(*this);
    }
}

void GarbageCollector::markObject(PomeObject* object) {
    if (object == nullptr || object->isMarked) return;
    object->isMarked = true;
    grayStack_.push_back(object);
}

void GarbageCollector::markValue(PomeValue& value) {
    value.mark(*this);
}

void GarbageCollector::markTable(std::map<PomeValue, PomeValue>& table) {
    for (auto& pair : table) {
        pair.first.mark(*this);
        pair.second.mark(*this);
    }
}

void sweepList(PomeObject** listHead, size_t& bytesAllocated) {
    PomeObject** object = listHead;
    while (*object) {
        if ((*object)->isMarked) {
            (*object)->isMarked = false;
            object = &(*object)->next;
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            bytesAllocated -= unreached->gcSize;
            delete unreached;
        }
    }
}

void GarbageCollector::sweep(bool minor) {
    if (!minor) {
        sweepList(&oldObjects_, bytesAllocated_);
    }

    // Reset youngBytesAllocated and rebuild it from survivors
    youngBytesAllocated_ = 0;
    PomeObject** object = &youngObjects_;
    while (*object) {
        if ((*object)->isMarked) {
            (*object)->isMarked = false;
            PomeObject* survivor = *object;

            survivor->age++;
            if (survivor->age >= 2) {
                // Promote to old generation
                *object = survivor->next;
                survivor->generation = 1;
                survivor->next = oldObjects_;
                oldObjects_ = survivor;
            } else {
                // Keep in young generation
                youngBytesAllocated_ += survivor->gcSize;
                object = &survivor->next;
            }
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            bytesAllocated_ -= unreached->gcSize;
            delete unreached;
        }
    }

    rememberedSet_.clear();

    if (!minor) {
        nextGC_ = bytesAllocated_ * 2;
        if (nextGC_ < 16 * 1024 * 1024) nextGC_ = 16 * 1024 * 1024;
    }
}

void GarbageCollector::addTemporaryRoot(PomeObject* obj) {
    tempRoots_.push_back(obj);
}

void GarbageCollector::removeTemporaryRoot(PomeObject* obj) {
    if (!tempRoots_.empty() && tempRoots_.back() == obj) {
        tempRoots_.pop_back();
    } else {
        for (auto it = tempRoots_.begin(); it != tempRoots_.end(); ++it) {
            if (*it == obj) {
                tempRoots_.erase(it);
                break;
            }
        }
    }
}
size_t GarbageCollector::getObjectCount() const
{
    size_t count = 0;
    PomeObject* obj = youngObjects_;
    while (obj) { count++; obj = obj->next; }
    obj = oldObjects_;
    while (obj) { count++; obj = obj->next; }
    return count;
}

std::string GarbageCollector::getInfo() const
{
    std::stringstream ss;
    ss << "Total: " << (bytesAllocated_ / 1024) << "KB, ";
    ss << "Young: " << (youngBytesAllocated_ / 1024) << "KB, ";
    ss << "Count: " << gcCount_;
    return ss.str();
}


} // namespace Pome
