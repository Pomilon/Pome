#include "pome_value.h"
#include "pome_chunk.h"
#include "pome_gc.h"
#include "pome_shape.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace Pome
{
    // --- PomeValue ---

    PomeValue::PomeValue() : value_(QNAN | TAG_NIL) {}
    PomeValue::PomeValue(std::monostate) : value_(QNAN | TAG_NIL) {}
    PomeValue::PomeValue(bool b) : value_(QNAN | (b ? TAG_TRUE : TAG_FALSE)) {}
    PomeValue::PomeValue(double d) { std::memcpy(&value_, &d, sizeof(double)); }
    PomeValue::PomeValue(PomeObject* obj) : value_(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)obj) {}

    bool PomeValue::isString() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::STRING;
    }

    bool PomeValue::isFunction() const
    {
        PomeObject* obj = asObject();
        return obj && (obj->type() == ObjectType::FUNCTION || obj->type() == ObjectType::NATIVE_FUNCTION);
    }

    bool PomeValue::isPomeFunction() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::FUNCTION;
    }

    bool PomeValue::isNativeFunction() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::NATIVE_FUNCTION;
    }

    bool PomeValue::isList() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::LIST;
    }

    bool PomeValue::isTable() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::TABLE;
    }

    bool PomeValue::isClass() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::CLASS;
    }

    bool PomeValue::isInstance() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::INSTANCE;
    }

    bool PomeValue::isModule() const { return isObject() && asObject()->type() == ObjectType::MODULE; }
    bool PomeValue::isTask() const { return isObject() && asObject()->type() == ObjectType::TASK; }
    bool PomeValue::isShape() const { return isObject() && asObject()->type() == ObjectType::SHAPE; }

    void PomeShape::markChildren(GarbageCollector& gc) {
        if (parent) gc.markObject(parent);
        propertyKey.mark(gc);
        for (auto const& [key, shape] : transitions) {
            key.mark(gc);
            gc.markObject(shape);
        }
    }
    
    int PomeShape::getIndex(PomeValue key) {
        PomeShape* current = this;
        while (current && current->parent) {
            if (current->propertyKey == key) return current->propertyIndex;
            current = current->parent;
        }
        return -1;
    }

    PomeShape* PomeShape::transition(GarbageCollector& gc, PomeValue key) {
        auto it = transitions.find(key);
        if (it != transitions.end()) return it->second;
        
        PomeShape* next = gc.allocate<PomeShape>(this, key, propertyIndex + 1);
        transitions[key] = next;
        return next;
    }

    void PomeInstance::markChildren(GarbageCollector& gc) {
        if (klass) gc.markObject(klass);
        if (shape) gc.markObject(shape);
        for (auto& val : properties) val.mark(gc);
    }

    const std::string &PomeValue::asString() const
    {
        return static_cast<PomeString*>(asObject())->getValue();
    }

    PomeFunction* PomeValue::asPomeFunction() const
    {
        return static_cast<PomeFunction*>(asObject());
    }

    NativeFunction* PomeValue::asNativeFunction() const
    {
        return static_cast<NativeFunction*>(asObject());
    }

    PomeList* PomeValue::asList() const
    {
        return static_cast<PomeList*>(asObject());
    }

    PomeTable* PomeValue::asTable() const
    {
        return static_cast<PomeTable*>(asObject());
    }

    PomeClass* PomeValue::asClass() const
    {
        return static_cast<PomeClass*>(asObject());
    }

    PomeShape* PomeValue::asShape() const
    {
        return static_cast<PomeShape*>(asObject());
    }

    PomeInstance* PomeValue::asInstance() const
    {
        return static_cast<PomeInstance*>(asObject());
    }

    PomeModule* PomeValue::asModule() const { return (PomeModule*)asObject(); }

    std::string PomeValue::toString() const
    {
        if (isNil()) return "nil";
        if (isTrue()) return "true";
        if (isFalse()) return "false";
        if (isNumber())
        {
            double d = asNumber();
            if (d == (long long)d) return std::to_string((long long)d);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision(6) << d;
            std::string s = ss.str();
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s.pop_back();
            return s;
        }
        if (isObject()) return asObject()->toString();
        return "unknown";
    }

    bool PomeValue::operator==(const PomeValue &other) const {
        if (value_ == other.value_) return true;
        if (isString() && other.isString()) return asString() == other.asString();
        return false;
    }

    bool PomeValue::operator<(const PomeValue &other) const {
        if (isNumber() && other.isNumber()) return asNumber() < other.asNumber();
        if (isString() && other.isString()) return asString() < other.asString();
        return value_ < other.value_;
    }

    bool PomeValue::operator<=(const PomeValue &other) const {
        return (*this < other) || (*this == other);
    }

    bool PomeValue::operator>(const PomeValue &other) const {
        return !(*this <= other);
    }

    bool PomeValue::operator>=(const PomeValue &other) const {
        return !(*this < other);
    }

    PomeValue PomeValue::deepCopy(GarbageCollector& targetGC, std::map<PomeObject*, PomeObject*>& copiedObjects) const {
        if (!isObject()) return *this;
        PomeObject* obj = asObject();
        if (copiedObjects.count(obj)) return PomeValue(copiedObjects[obj]);

        if (isString()) {
            PomeString* newStr = targetGC.allocate<PomeString>(asString());
            copiedObjects[obj] = newStr;
            return PomeValue(newStr);
        }
        if (isList()) {
            PomeList* oldList = asList();
            PomeList* newList = targetGC.allocateList();
            copiedObjects[obj] = newList;
            if (oldList->isUnboxed) {
                newList->isUnboxed = true;
                newList->unboxedElements = oldList->unboxedElements;
            } else {
                for (auto& val : oldList->elements) {
                    newList->elements.push_back(val.deepCopy(targetGC, copiedObjects));
                }
            }
            return PomeValue(newList);
        }
        if (isTable()) {
            PomeTable* oldTable = asTable();
            // Re-insert all elements to build a correct shape in the target isolate
            PomeTable* newTable = targetGC.allocate<PomeTable>(targetGC.getRootShape());
            copiedObjects[obj] = newTable;
            
            PomeShape* s = oldTable->shape;
            std::vector<PomeShape*> chain;
            while (s && s->parent) { chain.push_back(s); s = s->parent; }
            for (int i = (int)chain.size()-1; i >= 0; --i) {
                newTable->set(chain[i]->propertyKey.deepCopy(targetGC, copiedObjects), 
                               oldTable->properties[chain[i]->propertyIndex].deepCopy(targetGC, copiedObjects));
            }
            for (auto const& [key, val] : oldTable->backfill) {
                newTable->set(key.deepCopy(targetGC, copiedObjects), val.deepCopy(targetGC, copiedObjects));
            }
            return PomeValue(newTable);
        }
        return *this;
    }

    void PomeValue::mark(GarbageCollector& gc) const {
        if (isObject()) gc.markObject(asObject());
    }

    size_t PomeValue::hash() const {
        if (isString()) return std::hash<std::string>{}(asString());
        return std::hash<uint64_t>{}(value_);
    }

    PomeValue NativeFunction::call(const std::vector<PomeValue> &args) {
        return function_(args);
    }

    PomeTable::PomeTable(PomeShape* s) : shape(s) {}

    void PomeTable::set(PomeValue key, PomeValue value) {
        int index = shape ? shape->getIndex(key) : -1;
        if (index >= 0) {
            properties[index] = value;
        } else {
            backfill[key] = value;
        }
    }

    PomeValue PomeTable::get(PomeValue key) {
        int index = shape ? shape->getIndex(key) : -1;
        if (index >= 0) return properties[index];
        auto it = backfill.find(key);
        if (it != backfill.end()) return it->second;
        return PomeValue();
    }

    PomeInstance::PomeInstance(PomeClass* k, PomeShape* s) : klass(k), shape(s) {
        if (s) properties.resize(s->propertyIndex + 1);
    }

    void PomeInstance::set(PomeValue key, PomeValue value) {
        int index = shape ? shape->getIndex(key) : -1;
        if (index >= 0) {
            properties[index] = value;
        }
    }

    PomeValue PomeInstance::get(PomeValue key) {
        int index = shape ? shape->getIndex(key) : -1;
        if (index >= 0) return properties[index];
        return PomeValue();
    }

    std::vector<PomeValue> PomeTable::getSortedKeys() const {
        std::vector<PomeValue> keys;
        PomeShape* s = shape;
        while (s && s->parent) {
            keys.push_back(s->propertyKey);
            s = s->parent;
        }
        for (auto const& [key, val] : backfill) {
            keys.push_back(key);
        }
        std::sort(keys.begin(), keys.end());
        return keys;
    }

    std::string PomeTable::toString() const {
        std::string res = "{";
        bool first = true;
        
        PomeShape* current = shape;
        std::vector<PomeShape*> chain;
        while (current && current->parent) {
            chain.push_back(current);
            current = current->parent;
        }
        
        for (int i = (int)chain.size() - 1; i >= 0; --i) {
            if (!first) res += ", ";
            res += chain[i]->propertyKey.toString() + ": " + properties[chain[i]->propertyIndex].toString();
            first = false;
        }
        
        for (auto const& [key, val] : backfill) {
            if (!first) res += ", ";
            res += key.toString() + ": " + val.toString();
            first = false;
        }
        res += "}";
        return res;
    }

    void PomeList::markChildren(GarbageCollector& gc) {
        if (!isUnboxed) {
            for (auto& val : elements) val.mark(gc);
        }
    }

    std::string PomeList::toString() const {
        std::string res = "[";
        if (isUnboxed) {
            for (size_t i = 0; i < unboxedElements.size(); ++i) {
                if (i > 0) res += ", ";
                double d = unboxedElements[i];
                if (d == (long long)d) res += std::to_string((long long)d);
                else {
                    std::ostringstream ss;
                    ss << std::fixed << std::setprecision(6) << d;
                    std::string s = ss.str();
                    s.erase(s.find_last_not_of('0') + 1, std::string::npos);
                    if (s.back() == '.') s.pop_back();
                    res += s;
                }
            }
        } else {
            for (size_t i = 0; i < elements.size(); ++i) {
                if (i > 0) res += ", ";
                res += elements[i].toString();
            }
        }
        res += "]";
        return res;
    }

    void PomeList::tryUnbox() {
        if (isUnboxed || elements.empty()) return;

        // Check if all are numbers
        for (const auto& v : elements) {
            if (!v.isNumber()) return;
        }

        isUnboxed = true;
        unboxedElements.reserve(elements.size());
        for (const auto& v : elements) {
            unboxedElements.push_back(v.asNumber());
        }
        elements.clear();
        elements.shrink_to_fit();
    }

    void PomeList::box() {
        if (!isUnboxed) return;

        elements.reserve(unboxedElements.size());
        for (double d : unboxedElements) {
            elements.push_back(PomeValue(d));
        }

        unboxedElements.clear();
        unboxedElements.shrink_to_fit();
        isUnboxed = false;
    }

    void PomeList::ensureCapacity(size_t capacity) {
        if (isUnboxed) {
            if (capacity > unboxedElements.capacity()) {
                unboxedElements.reserve(capacity);
            }
        } else {
            if (capacity > elements.capacity()) {
                elements.reserve(capacity);
            }
        }
    }

    void PomeList::push(PomeValue val) {
        if (isUnboxed) {
            if (val.isNumber()) {
                unboxedElements.push_back(val.asNumber());
            } else {
                box();
                elements.push_back(val);
            }
        } else {
            elements.push_back(val);
        }
    }

    void PomeTable::markChildren(GarbageCollector& gc) {
        if (shape) gc.markObject(shape);
        for (auto& val : properties) val.mark(gc);
        for (auto const& [key, val] : backfill) {
            key.mark(gc);
            val.mark(gc);
        }
    }

    void PomeClass::markChildren(GarbageCollector& gc) {
        if (superclass) gc.markObject(superclass);
        if (module) gc.markObject(module);
        if (classShape) gc.markObject(classShape);
        for (auto const& [name, method] : methods) {
            gc.markObject(method);
        }
    }

    PomeFunction* PomeClass::findMethod(const std::string &name)
    {
        auto it = methods.find(name);
        if (it != methods.end())
        {
            return it->second;
        }
        if (superclass) {
            return superclass->findMethod(name);
        }
        return nullptr;
    }

    PomeFunction::PomeFunction() : chunk(std::make_shared<Chunk>()) {}

    void PomeUpvalue::markChildren(GarbageCollector& gc) {
        if (location) location->mark(gc);
    }

    void PomeFunction::markChildren(GarbageCollector& gc) {
        if (module) gc.markObject(module);
        if (klass) gc.markObject(klass);
        for (auto* uv : upvalues) {
            gc.markObject(uv);
        }
        if (chunk) {
            for (auto& val : chunk->constants) {
                val.mark(gc);
            }
        }
    }

    void PomeModule::markChildren(GarbageCollector& gc) {
        for (auto const& [key, val] : exports) {
            key.mark(gc); 
            val.mark(gc); 
        }
        for (auto const& [key, val] : variables) {
            key.mark(gc); 
            val.mark(gc); 
        }
    }

    void PomeThread::markChildren(GarbageCollector& gc) {
        result.mark(gc);
    }

    void PomeTask::markChildren(GarbageCollector& gc) {
        if (function) gc.markObject(function);
        for (auto& arg : args) arg.mark(gc);
        result.mark(gc);
    }

} // namespace Pome
