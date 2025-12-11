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
    
    /**
     * Trace children
     */
    switch (object->type()) {
        case ObjectType::STRING:
        case ObjectType::NATIVE_FUNCTION:
            /**
             * No children
             */
            break;
            
        case ObjectType::FUNCTION: {
            PomeFunction* fn = static_cast<PomeFunction*>(object);
            if (fn->closureEnv) markObject(fn->closureEnv);
            break;
        }
        
        case ObjectType::LIST: {
            PomeList* list = static_cast<PomeList*>(object);
            for (auto& val : list->elements) {
                markValue(val);
            }
            break;
        }
        
        case ObjectType::TABLE: {
            PomeTable* table = static_cast<PomeTable*>(object);
            markTable(table->elements);
            break;
        }
        
        case ObjectType::CLASS: {
            PomeClass* klass = static_cast<PomeClass*>(object);
            for (auto& pair : klass->methods) {
                markObject(pair.second);
            }
            break;
        }
        
        case ObjectType::INSTANCE: {
            PomeInstance* instance = static_cast<PomeInstance*>(object);
            markObject(instance->klass);
            for (auto& pair : instance->fields) {
                markValue(pair.second);
            }
            break;
        }
        
        case ObjectType::MODULE: {
            PomeModule* module = static_cast<PomeModule*>(object);
            markTable(module->exports);
            break;
        }
        
        case ObjectType::ENVIRONMENT: {
            Environment* env = static_cast<Environment*>(object);
            if (env->getParent()) markObject(env->getParent());
            markEnvironmentStore(env->getStore());
            break;
        }

        default:
            break;
    }
}

void GarbageCollector::markValue(PomeValue& value) {
    /**
     * Check for pointer types before marking
     */
    if (value.isString() || value.isFunction() || value.isList() || value.isTable() || 
        value.isClass() || value.isInstance() || value.isEnvironment() || value.isPomeFunction() || value.isNativeFunction()) {
        markObject(value.asObject());
    }
}

void GarbageCollector::markTable(std::map<PomeValue, PomeValue>& table) {
    for (auto& pair : table) {
        PomeValue key = pair.first; // Copy
        markValue(key);
        markValue(pair.second);
    }
}

void GarbageCollector::markEnvironmentStore(std::map<std::string, PomeValue>& store) {
    for (auto& pair : store) {
        markValue(pair.second);
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
