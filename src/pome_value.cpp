#include "../include/pome_value.h"
#include "../include/pome_environment.h"
#include "../include/pome_gc.h"
#include "../include/pome_chunk.h" // Added
#include <iostream>
#include <sstream>
#include <algorithm>

namespace Pome
{
    PomeFunction::PomeFunction() : chunk(std::make_unique<Chunk>()) {}

    /**
     * PomeValue implementation
     */
    PomeValue NativeFunction::call(const std::vector<PomeValue> &args)
    {
        return function_(args);
    }

    /**
     * --- PomeValue Implementation (NaN Boxing) ---
     */

    PomeValue::PomeValue() : value_(QNAN | TAG_NIL) {}
    PomeValue::PomeValue(std::monostate) : value_(QNAN | TAG_NIL) {}
    
    PomeValue::PomeValue(bool b) : value_(b ? (QNAN | TAG_TRUE) : (QNAN | TAG_FALSE)) {}
    
    PomeValue::PomeValue(double d)
    {
        std::memcpy(&value_, &d, sizeof(double));
    }
    
    PomeValue::PomeValue(PomeObject* obj)
    {
        value_ = (uint64_t)(uintptr_t)obj | QNAN | SIGN_BIT;
    }

    /**
     * Type checking
     */
    // isNil, isBool, isNumber, isObject moved to header

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

    bool PomeValue::isEnvironment() const
    {
        PomeObject* obj = asObject();
        return obj && obj->type() == ObjectType::ENVIRONMENT;
    }

    /**
     * Getters
     */
    // asBool, asNumber, asObject moved to header

    const std::string &PomeValue::asString() const
    {
        auto obj = static_cast<PomeString*>(asObject());
        return obj->getValue();
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

    Environment* PomeValue::asEnvironment() const
    {
        return static_cast<Environment*>(asObject());
    }

    std::string PomeValue::toString() const
    {
        if (isNil())
            return "nil";
        if (isBool())
            return asBool() ? "true" : "false";
        if (isNumber())
        {
            std::stringstream ss;
            double d = asNumber();
            if (d == static_cast<long long>(d))
            {
                ss << static_cast<long long>(d);
            }
            else
            {
                ss << d;
            }
            return ss.str();
        }
        if (isObject())
        {
            return asObject()->toString();
        }
        return "unknown";
    }

    bool PomeValue::operator==(const PomeValue &other) const
    {
        // Fast path: exact bit equality
        if (value_ == other.value_) return true;

        // If both are numbers, use double equality (handles -0.0 == 0.0)
        if (isNumber() && other.isNumber()) {
            return asNumber() == other.asNumber();
        }

        // If types are different, they are not equal (unless we want loose equality, but Pome seems strict-ish)
        // Check if one is object and other is not
        if (isObject() != other.isObject()) return false;
        
        // If both are objects (and bits were different), we might need deep comparison for Strings
        if (isObject()) {
            PomeObject* obj1 = asObject();
            PomeObject* obj2 = other.asObject();
            
            if (obj1->type() == ObjectType::STRING && obj2->type() == ObjectType::STRING)
            {
                return static_cast<PomeString*>(obj1)->getValue() ==
                       static_cast<PomeString*>(obj2)->getValue();
            }
        }

        return false;
    }

    bool PomeValue::operator<(const PomeValue &other) const
    {
        // 1. Same types
        if (isNumber() && other.isNumber()) return asNumber() < other.asNumber();
        if (isString() && other.isString()) return asString() < other.asString();
        if (isBool() && other.isBool()) return asBool() < other.asBool();
        if (isNil() && other.isNil()) return false;
        
        // 2. Different types: use a stable order
        // Order: Nil < Bool < Number < String < List < Table < Function < Class < Instance
        auto typeOrder = [](const PomeValue& v) {
            if (v.isNil()) return 0;
            if (v.isBool()) return 1;
            if (v.isNumber()) return 2;
            if (v.isString()) return 3;
            if (v.isList()) return 4;
            if (v.isTable()) return 5;
            if (v.isFunction()) return 6;
            if (v.isClass()) return 7;
            if (v.isInstance()) return 8;
            return 9;
        };
        
        int t1 = typeOrder(*this);
        int t2 = typeOrder(other);
        if (t1 != t2) return t1 < t2;
        
        // Same type but complex object: compare pointers
        return value_ < other.value_;
    }

    void PomeValue::mark(GarbageCollector& gc) const {
        if (isObject()) {
            gc.markObject(asObject());
        }
    }

    /**
     * Implementation of toString for types that were previously in header or split
     */
    std::string PomeList::toString() const
    {
        std::string s = "[";
        for (size_t i = 0; i < elements.size(); ++i)
        {
            s += elements[i].toString();
            if (i < elements.size() - 1)
                s += ", ";
        }
        s += "]";
        return s;
    }
    void PomeList::markChildren(GarbageCollector& gc) {
        for (PomeValue& element : elements) {
            element.mark(gc);
        }
    }


    std::string PomeTable::toString() const
    {
        std::string s = "{";
        size_t i = 0;
        for (const auto &pair : elements)
        {
            s += pair.first.toString() + ": " + pair.second.toString();
            if (i < elements.size() - 1)
                s += ", ";
            i++;
        }
        s += "}";
        return s;
    }
    void PomeTable::markChildren(GarbageCollector& gc) {
        for (auto const& [key, val] : elements) {
            key.mark(gc);
            val.mark(gc);
        }
    }

    /**
     * Class Definition Object
     */
    // PomeClass has no children directly managed by GC other than methods map
    void PomeClass::markChildren(GarbageCollector& gc) {
        for (auto const& [name, func] : methods) {
            if (func) { // func is PomeFunction*
                gc.markObject(func);
            }
        }
    }

    /**
     * Class Instance Object
     */
    // PomeInstance methods
    void PomeInstance::markChildren(GarbageCollector& gc) {
        if (klass) {
            gc.markObject(klass);
        }
        for (auto const& [name, val] : fields) {
            val.mark(gc); // val is PomeValue
        }
    }

    PomeValue PomeInstance::get(const std::string &name)
    {
        auto it = fields.find(name);
        if (it != fields.end())
        {
            return it->second;
        }
        return PomeValue(); // nil
    }

    void PomeInstance::set(const std::string &name, PomeValue value)
    {
        fields[name] = value;
    }

    /**
     * User-defined Function Object
     */
    void PomeFunction::markChildren(GarbageCollector& gc) {
        if (closureEnv) {
            gc.markObject(closureEnv);
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
            key.mark(gc); // Key is PomeValue
            val.mark(gc); // Value is PomeValue
        }
    }

} // namespace Pome
