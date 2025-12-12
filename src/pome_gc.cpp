#include "../include/pome_gc.h"
#include "../include/pome_interpreter.h"
#include <iostream>

namespace Pome {

GarbageCollector::GarbageCollector() {}

void GarbageCollector::setInterpreter(Interpreter* interpreter) {
    interpreter_ = interpreter;
}

void GarbageCollector::collect() {
    mark();
    sweep();
}

void GarbageCollector::mark() {
    /**
     * 1. Mark roots from interpreter
     */
    if (interpreter_) {
        interpreter_->markRoots();
    }
    
    /**
     * 2. Mark temporary roots
     */
    for (auto* obj : tempRoots_) {
        markObject(obj);
    }
}

void GarbageCollector::markObject(PomeObject* object) {
    if (object == nullptr || object->isMarked) return;
    
    object->isMarked = true;
    object->markChildren(*this); // Call virtual markChildren method
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

void GarbageCollector::sweep() {
    PomeObject** object = &head_;
    while (*object) {
        if ((*object)->isMarked) {
            (*object)->isMarked = false;
            object = &(*object)->next;
        } else {
            PomeObject* unreached = *object;
            *object = unreached->next;
            
            /**
             * Free memory
             */
            bytesAllocated_ -= unreached->gcSize;
            delete unreached;
        }
    }
    
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
    /**
     * Simple remove from back optimization if LIFO, else search
     */
    if (!tempRoots_.empty() && tempRoots_.back() == obj) {
        tempRoots_.pop_back();
    } else {
        /**
         * Search and remove
         */
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
    PomeObject* obj = head_;
    while (obj) {
        count++;
        obj = obj->next;
    }
    return count;
}

} // namespace Pome
