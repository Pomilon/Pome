#include "../include/pome_gc.h"
#include "../include/pome_vm.h"
#include "../include/pome_value.h"
#include <iostream>
#include <sstream>
#include <fstream>
#include <unistd.h>

namespace Pome {

GarbageCollector::GarbageCollector() {}

void GarbageCollector::setVM(VM* vm) {
    vm_ = vm;
}

PomeShape* GarbageCollector::getRootShape() const {
    if (vm_) return vm_->getRootShape();
    return nullptr;
}

PomeList* GarbageCollector::allocateList() {
    if (youngBytesAllocated_ > nextMinorGC_) collect(true);
    if (bytesAllocated_ > nextGC_) collect(false);

    PomeList* list = nullptr;
    if (!listPool_.empty()) {
        list = static_cast<PomeList*>(listPool_.back());
        listPool_.pop_back();
        list->listType = ListType::MIXED;
        if (list->unboxedData) {
            free(list->unboxedData);
            list->unboxedData = nullptr;
            list->unboxedCount = 0;
            list->unboxedCapacity = 0;
        }
        list->gcSize = sizeof(PomeList) + list->extraSize();
    } else {
        list = new PomeList();
        list->listType = ListType::MIXED;
        list->gcSize = sizeof(PomeList);
    }

    list->generation = 0;
    list->age = 0;
    list->isMarked = false;
    list->next = youngObjects_;
    youngObjects_ = list;

    bytesAllocated_ += list->gcSize;
    youngBytesAllocated_ += list->gcSize;

    return list;
}

void GarbageCollector::updateSize(PomeObject* obj, size_t oldSize, size_t newSize) {
    if (newSize > oldSize) {
        size_t diff = newSize - oldSize;
        bytesAllocated_ += diff;
        if (obj->generation == 0) {
            youngBytesAllocated_ += diff;
            if (youngBytesAllocated_ > nextMinorGC_) pendingGC = true;
        } else {
            if (bytesAllocated_ > nextGC_) pendingGC = true;
        }
        obj->gcSize = newSize;
        // std::cout << "[GC_ALLOC] Grow obj type " << (int)obj->type() << " " << oldSize << " -> " << newSize << " Total=" << bytesAllocated_/1024 << "KB" << std::endl;
    } else if (oldSize > newSize) {
        size_t diff = oldSize - newSize;
        if (bytesAllocated_ >= diff) bytesAllocated_ -= diff;
        else bytesAllocated_ = 0;

        if (obj->generation == 0) {
            if (youngBytesAllocated_ >= diff) youngBytesAllocated_ -= diff;
            else youngBytesAllocated_ = 0;
        }
        obj->gcSize = newSize;
    }
}

void GarbageCollector::collect(bool minor) {
    gcCount_++;
    size_t before = bytesAllocated_;
    mark(minor);
    sweep(minor);
    // std::cout << "[GC_EVENT] " << (minor ? "Minor" : "Major") << " freed=" << (before - bytesAllocated_)/1024 << "KB remaining=" << bytesAllocated_/1024 << "KB RSS=" << getRSS() << "KB" << std::endl;
}

void GarbageCollector::writeBarrier(PomeObject* parent, const PomeValue& child) {
    if (parent && parent->generation == 1 && child.isObject()) {
        PomeObject* childObj = child.asObject();
        if (childObj && childObj->generation == 0) {
            for (auto* obj : rememberedSet_) {
                if (obj == parent) return;
            }
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

void GarbageCollector::markValue(const PomeValue& value) {
    value.mark(*this);
}

void GarbageCollector::markTable(std::map<PomeValue, PomeValue>& table) {
    for (auto& pair : table) {
        pair.first.mark(*this);
        pair.second.mark(*this);
    }
}

void sweepList(PomeObject** listHead, size_t& bytesAllocated, std::vector<PomeObject*>& listPool, bool resetMark) {
    PomeObject** object = listHead;
    while (*object) {
        if ((*object)->isMarked) {
            if (resetMark) (*object)->isMarked = false;
            object = &(*object)->next;
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            bytesAllocated -= unreached->gcSize;
            if (unreached->type() == ObjectType::LIST && listPool.size() < 1000) {
                PomeList* lst = static_cast<PomeList*>(unreached);
                if (lst->unboxedData) free(lst->unboxedData);
                lst->unboxedData = nullptr;
                lst->unboxedCount = 0;
                lst->unboxedCapacity = 0;
                lst->listType = ListType::MIXED;
                lst->elements.clear();
                if (lst->elements.capacity() > 256) lst->elements.shrink_to_fit();
                listPool.push_back(lst);
            } else {
                delete unreached;
            }
        }
    }
}

void GarbageCollector::sweep(bool minor) {
    if (!minor) {
        sweepList(&oldObjects_, bytesAllocated_, listPool_, true);
    } else {
        PomeObject* obj = oldObjects_;
        while (obj) {
            obj->isMarked = false;
            obj = obj->next;
        }
    }

    youngBytesAllocated_ = 0;
    PomeObject** object = &youngObjects_;
    while (*object) {
        if ((*object)->isMarked) {
            (*object)->isMarked = false;
            PomeObject* survivor = *object;
            survivor->age++;
            if (survivor->age >= 2) {
                *object = survivor->next;
                survivor->generation = 1;
                survivor->next = oldObjects_;
                oldObjects_ = survivor;
            } else {
                youngBytesAllocated_ += survivor->gcSize;
                object = &survivor->next;
            }
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            bytesAllocated_ -= unreached->gcSize;
            if (unreached->type() == ObjectType::LIST && listPool_.size() < 1000) {
                PomeList* lst = static_cast<PomeList*>(unreached);
                if (lst->unboxedData) free(lst->unboxedData);
                lst->unboxedData = nullptr;
                lst->unboxedCount = 0;
                lst->unboxedCapacity = 0;
                lst->listType = ListType::MIXED;
                lst->elements.clear();
                if (lst->elements.capacity() > 256) lst->elements.shrink_to_fit();
                listPool_.push_back(lst);
            } else {
                delete unreached;
            }
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
size_t GarbageCollector::getObjectCount() const {
    size_t count = 0;
    PomeObject* obj = youngObjects_;
    while (obj) { count++; obj = obj->next; }
    obj = oldObjects_;
    while (obj) { count++; obj = obj->next; }
    return count;
}

std::string GarbageCollector::getInfo() const {
    std::stringstream ss;
    ss << "Total: " << (bytesAllocated_ / 1024) << "KB, ";
    ss << "Young: " << (youngBytesAllocated_ / 1024) << "KB, ";
    ss << "Count: " << gcCount_;
    return ss.str();
}

void GarbageCollector::dumpHeap() const {
    std::map<ObjectType, int> youngCounts;
    std::map<ObjectType, int> oldCount;
    size_t youngBytes = 0;
    size_t oldBytes = 0;
    auto scan = [&](PomeObject* head, std::map<ObjectType, int>& counts, size_t& bytes) {
        PomeObject* obj = head;
        while (obj) {
            counts[obj->type()]++;
            bytes += obj->gcSize;
            obj = obj->next;
        }
    };
    scan(youngObjects_, youngCounts, youngBytes);
    scan(oldObjects_, oldCount, oldBytes);
    std::cout << "--- HEAP DUMP --- RSS=" << getRSS() << "KB" << std::endl;
    std::cout << "Young Objects: " << youngBytes / 1024 << " KB" << std::endl;
    for (auto const& [type, count] : youngCounts) std::cout << "  - Type " << (int)type << ": " << count << std::endl;
    std::cout << "Old Objects: " << oldBytes / 1024 << " KB" << std::endl;
    for (auto const& [type, count] : oldCount) std::cout << "  - Type " << (int)type << ": " << count << std::endl;
    std::cout << "Total Managed: " << bytesAllocated_ / 1024 << " KB" << std::endl;
    std::cout << "List Pool Size: " << listPool_.size() << std::endl;
    std::cout << "-----------------" << std::endl;
}

} // namespace Pome
