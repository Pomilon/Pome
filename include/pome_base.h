#ifndef POME_BASE_H
#define POME_BASE_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include <cstring>
#include <cstdint>
#include <variant>
#include <thread>
#include <atomic>
#include "pome_ast.h"

namespace Pome {

    class GarbageCollector;
    class Chunk;
    class Statement;

    enum class ObjectType {
        STRING,
        FUNCTION,
        NATIVE_FUNCTION,
        LIST,
        TABLE,
        CLASS,
        INSTANCE,
        MODULE,
        THREAD,
        TASK,
        UPVALUE,
        SHAPE,
        NATIVE_OBJECT 
    };

    class PomeObject {
    public:
        virtual ~PomeObject() = default;
        virtual ObjectType type() const = 0;
        virtual std::string toString() const = 0;

        bool isMarked = false;
        uint8_t generation = 0; 
        uint8_t age = 0;        
        size_t gcSize = 0;      
        PomeObject* next = nullptr;

        virtual void markChildren(GarbageCollector& gc) {}
        virtual size_t extraSize() const { return 0; }
    };

    class PomeString;
    class PomeFunction;
    class NativeFunction;
    class PomeList;
    class PomeTable;
    class PomeClass;
    class PomeInstance;
    class PomeModule;
    class PomeUpvalue;
    class PomeThread;
    class PomeTask;
    class PomeShape;

    class PomeValue;
    using ModuleLoader = std::function<PomeValue(const std::string&)>;
    using NativeFn = std::function<PomeValue(const std::vector<PomeValue> &)>;

    class PomeValue {
    public:
        PomeValue();
        explicit PomeValue(std::monostate);
        explicit PomeValue(bool b);
        explicit PomeValue(double d);
        explicit PomeValue(PomeObject* obj);

        inline bool isNil() const { return value_ == (QNAN | TAG_NIL); }
        inline bool isBool() const { return value_ == (QNAN | TAG_TRUE) || value_ == (QNAN | TAG_FALSE); }
        inline bool isNumber() const { return (value_ & QNAN) != QNAN; }
        inline bool isObject() const { return (value_ & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT); }
        inline bool isTrue() const { return value_ == (QNAN | TAG_TRUE); }
        inline bool isFalse() const { return value_ == (QNAN | TAG_FALSE); }
        
        bool isString() const;
        bool isFunction() const;
        bool isPomeFunction() const;
        bool isNativeFunction() const;
        bool isList() const;
        bool isTable() const;
        bool isClass() const;
        bool isInstance() const;
        bool isModule() const;
        bool isTask() const;
        bool isShape() const;

        inline bool asBool() const {
            if (value_ == (QNAN | TAG_TRUE) || value_ == (QNAN | TAG_FALSE))
                return value_ == (QNAN | TAG_TRUE);
            if (isNumber())
                return asNumber() != 0.0;
            if (value_ == (QNAN | TAG_NIL))
                return false;
            return true;
        }

        inline double asNumber() const {
            double d;
            std::memcpy(&d, &value_, sizeof(double));
            return d;
        }
        
        inline PomeObject* asObject() const {
            if (isObject()) {
                return (PomeObject*)(uintptr_t)(value_ & ~(SIGN_BIT | QNAN));
            }
            return nullptr;
        }

        const std::string &asString() const;
        PomeFunction* asPomeFunction() const;
        NativeFunction* asNativeFunction() const;
        PomeList* asList() const;
        PomeTable* asTable() const;
        PomeClass* asClass() const;
        PomeInstance* asInstance() const;
        PomeModule* asModule() const;
        PomeShape* asShape() const;

        std::string toString() const;
        bool operator==(const PomeValue &other) const;
        bool operator!=(const PomeValue &other) const { return !(*this == other); }
        bool operator<(const PomeValue &other) const;
        bool operator<=(const PomeValue &other) const;
        bool operator>(const PomeValue &other) const;
        bool operator>=(const PomeValue &other) const;

        PomeValue deepCopy(GarbageCollector& targetGC, std::map<PomeObject*, PomeObject*>& copiedObjects) const;
        void mark(GarbageCollector& gc) const;
        size_t hash() const;
        uint64_t getRaw() const { return value_; }

    private:
        uint64_t value_;
        static constexpr uint64_t QNAN = 0x7ffc000000000000;
        static constexpr uint64_t SIGN_BIT = 0x8000000000000000;
        static constexpr uint64_t TAG_NIL = 1;
        static constexpr uint64_t TAG_FALSE = 2;
        static constexpr uint64_t TAG_TRUE = 3;
    };

} // namespace Pome

namespace std {
    template<>
    struct hash<Pome::PomeValue> {
        size_t operator()(const Pome::PomeValue& v) const {
            return v.hash();
        }
    };
}

#endif // POME_BASE_H
