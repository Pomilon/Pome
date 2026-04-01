#include "pome_value.h"
#include "pome_chunk.h"
#include "pome_gc.h"
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

    bool PomeValue::isModule() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::MODULE;
    }

    bool PomeValue::isTask() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::TASK;
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

    PomeInstance* PomeValue::asInstance() const
    {
        return static_cast<PomeInstance*>(asObject());
    }

    PomeModule* PomeValue::asModule() const
    {
        return static_cast<PomeModule*>(asObject());
    }

    std::string PomeValue::toString() const
    {
        if (isNil()) return "nil";
        if (isBool()) return asBool() ? "true" : "false";
        if (isNumber()) {
            double d = asNumber();
            if (std::floor(d) == d) return std::to_string((long long)d);
            std::ostringstream oss;
            oss << std::setprecision(14) << d;
            return oss.str();
        }
        if (isObject()) return asObject()->toString();
        return "???";
    }

    bool PomeValue::operator==(const PomeValue &other) const
    {
        if (value_ == other.value_) return true;
        if (isString() && other.isString()) {
            return asString() == other.asString();
        }
        return false;
    }

    int typeOrder(const PomeValue& v) {
        if (v.isNil()) return 0;
        if (v.isBool()) return 1;
        if (v.isNumber()) return 2;
        if (v.isString()) return 3;
        return 4;
    }

    bool PomeValue::operator<(const PomeValue &other) const
    {
        if (isNumber() && other.isNumber()) return asNumber() < other.asNumber();
        if (isString() && other.isString()) return asString() < other.asString();
        
        int t1 = typeOrder(*this);
        int t2 = typeOrder(other);
        if (t1 != t2) return t1 < t2;

        return value_ < other.value_;
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
            for (auto& val : oldList->elements) {
                newList->elements.push_back(val.deepCopy(targetGC, copiedObjects));
            }
            return PomeValue(newList);
        }
        if (isTable()) {
            PomeTable* oldTable = asTable();
            PomeTable* newTable = targetGC.allocate<PomeTable>();
            copiedObjects[obj] = newTable;
            for (auto const& [key, val] : oldTable->elements) {
                newTable->elements[key.deepCopy(targetGC, copiedObjects)] = val.deepCopy(targetGC, copiedObjects);
            }
            return PomeValue(newTable);
        }
        if (isPomeFunction()) {
            PomeFunction* oldFunc = asPomeFunction();
            PomeFunction* newFunc = targetGC.allocate<PomeFunction>();
            copiedObjects[obj] = newFunc;
            newFunc->name = oldFunc->name;
            newFunc->parameters = oldFunc->parameters;
            newFunc->chunk = oldFunc->chunk; // Bytecode is shared (read-only)
            newFunc->upvalueCount = oldFunc->upvalueCount;
            newFunc->isAsync = oldFunc->isAsync;
            // Note: Upvalues in isolates are tricky. For now, we deep-copy closed upvalues
            // but isolates generally shouldn't share open upvalues.
            for (auto* uv : oldFunc->upvalues) {
                // This is a simplification
                PomeUpvalue* newUv = targetGC.allocate<PomeUpvalue>(&newUv->closedValue);
                newUv->closedValue = (*uv->location).deepCopy(targetGC, copiedObjects);
                newFunc->upvalues.push_back(newUv);
            }
            return PomeValue(newFunc);
        }
        
        return *this; // Other types not yet fully supported for deep copy
    }

    void PomeValue::mark(GarbageCollector& gc) const {
        if (isObject()) gc.markObject(asObject());
    }

    size_t PomeValue::hash() const {
        if (isString()) return std::hash<std::string>{}(asString());
        return std::hash<uint64_t>{}(value_);
    }

    PomeValue NativeFunction::call(const std::vector<PomeValue> &args)
    {
        return function_(args);
    }

    // --- Objects ---

    void PomeList::markChildren(GarbageCollector& gc) {
        for (auto& val : elements) val.mark(gc);
    }

    std::string PomeList::toString() const {
        std::string res = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            res += elements[i].toString();
            if (i < elements.size() - 1) res += ", ";
        }
        res += "]";
        return res;
    }

    void PomeTable::markChildren(GarbageCollector& gc) {
        for (auto const& [key, val] : elements) {
            key.mark(gc);
            val.mark(gc);
        }
    }

    std::string PomeTable::toString() const {
        std::string res = "{";
        size_t i = 0;
        for (auto const& [key, val] : elements) {
            res += key.toString() + ": " + val.toString();
            if (++i < elements.size()) res += ", ";
        }
        res += "}";
        return res;
    }

    void PomeClass::markChildren(GarbageCollector& gc) {
        if (superclass) gc.markObject(superclass);
        if (module) gc.markObject(module);
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

    void PomeInstance::markChildren(GarbageCollector& gc) {
        gc.markObject(klass);
        for (auto const& [name, val] : fields) {
            val.mark(gc);
        }
        if (fieldsArray) {
            for (uint8_t i = 0; i < fieldCount; ++i) {
                fieldsArray[i].mark(gc);
            }
        }
    }

    void PomeInstance::set(const std::string &name, PomeValue value)
    {
        if (fieldsArray) {
            auto it = klass->fieldNames.find(name);
            if (it != klass->fieldNames.end()) {
                fieldsArray[it->second] = value;
                return;
            }
        }
        fields[name] = value;
    }

    PomeValue PomeInstance::get(const std::string &name) {
        if (fieldsArray) {
            auto it = klass->fieldNames.find(name);
            if (it != klass->fieldNames.end()) {
                return fieldsArray[it->second];
            }
        }
        auto it = fields.find(name);
        if (it != fields.end()) return it->second;
        return PomeValue();
    }

    void PomeInstance::setFieldsArray(GarbageCollector& gc, uint8_t count)
    {
        if (fieldsArray) delete[] fieldsArray;
        fieldCount = count;
        fieldsArray = new PomeValue[count];
        gc.updateSize(this, gcSize, sizeof(PomeInstance) + count * sizeof(PomeValue));
    }

    PomeFunction::PomeFunction() : chunk(std::make_shared<Chunk>()) {}

    /**
     * Upvalue Object
     */
    void PomeUpvalue::markChildren(GarbageCollector& gc) {
        if (location) location->mark(gc);
    }

    /**
     * User-defined Function Object
     */
    void PomeFunction::markChildren(GarbageCollector& gc) {
        for (auto* uv : upvalues) {
            gc.markObject(uv);
        }
        if (chunk) {
            for (auto& val : chunk->constants) {
                val.mark(gc);
            }
        }
    }

    /**
     * Module Object
     */
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
