#include "../include/pome_gc.h"
#include "../include/pome_interpreter.h"
#include "../include/pome_vm.h" // Added
#include <iostream>

namespace Pome {

GarbageCollector::GarbageCollector() {}

void GarbageCollector::setInterpreter(Interpreter* interpreter) {
    interpreter_ = interpreter;
}

void GarbageCollector::setVM(VM* vm) {
    vm_ = vm;
}

void GarbageCollector::collect() {
    // For now, always do Major GC (Full Collection)
    // TODO: Implement Minor GC logic
    mark();
    sweep();
}

void GarbageCollector::writeBarrier(PomeObject* parent, PomeValue& child) {
    if (parent && parent->generation == 1 && child.isObject()) {
        PomeObject* childObj = child.asObject();
        if (childObj && childObj->generation == 0) {
            rememberedSet_.push_back(parent);
        }
    }
}

void GarbageCollector::mark() {
    /**
     * 1. Mark roots from interpreter
     */
    if (interpreter_) {
        interpreter_->markRoots();
    }
    
    /**
     * 2. Mark roots from VM
     */
    if (vm_) {
        vm_->markRoots();
    }
    
    /**
     * 3. Mark temporary roots
     */
    for (auto* obj : tempRoots_) {
        markObject(obj);
    }
    
    // Remembered Set should also be roots for Minor GC, but for Major GC they are just part of the graph.
    
    traceReferences(); // Process stack iteratively
}

void GarbageCollector::traceReferences() {
    while (!grayStack_.empty()) {
        PomeObject* object = grayStack_.back();
        grayStack_.pop_back();
        object->markChildren(*this);
    }
}

void GarbageCollector::markObject(PomeObject* object) {
    if (object == nullptr || object->isMarked) return;
    
    object->isMarked = true;
    grayStack_.push_back(object); // Push to stack instead of recursing immediately
}

void GarbageCollector::markValue(PomeValue& value) {
    value.mark(*this); // Delegate to PomeValue::mark
}

void GarbageCollector::markTable(std::map<PomeValue, PomeValue>& table) {
    for (auto& pair : table) {
        pair.first.mark(*this); // Mark key
        pair.second.mark(*this); // Mark value
    }
}

void GarbageCollector::markEnvironmentStore(std::map<std::string, PomeValue>& store) {
    for (auto& pair : store) {
        pair.second.mark(*this); // Mark value
    }
}

// Helper to sweep a specific list
void sweepList(PomeObject** listHead, size_t& bytesAllocated) {
    PomeObject** object = listHead;
    while (*object) {
        if ((*object)->isMarked) {
            (*object)->isMarked = false;
            // Promotion Logic: Simple age? Or just keep in same list?
            // For true Generational, we move from Young to Old here if it survives.
            // Let's implement promotion later.
            object = &(*object)->next;
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            
            bytesAllocated -= unreached->gcSize;
            delete unreached;
        }
    }
}

void GarbageCollector::sweep() {
    // 1. Sweep Old Gen first
    sweepList(&oldObjects_, bytesAllocated_);

    // 2. Sweep Young Gen and Promote survivors
    PomeObject** object = &youngObjects_;
    while (*object) {
        if ((*object)->isMarked) {
            (*object)->isMarked = false;
            
            // Promote to Old Gen
            PomeObject* survivor = *object;
            *object = survivor->next; // Remove from Young
            
            survivor->generation = 1; // Mark as Old
            survivor->next = oldObjects_; // Add to Old
            oldObjects_ = survivor;
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            
            bytesAllocated_ -= unreached->gcSize;
            delete unreached;
        }
    }
    
    // 3. Clear remembered set
    rememberedSet_.clear();
    
    /**
     * Adjust nextGC limit
     */
    if (bytesAllocated_ > 0) {
        nextGC_ = bytesAllocated_ * 2;
    } else {
        nextGC_ = 1024 * 1024;
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

} // namespace Pome
