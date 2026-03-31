#ifndef POME_VALUE_H
#define POME_VALUE_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include <cstring> // For memcpy
#include <cstdint> // For uint64_t, uintptr_t
#include <variant> // For std::monostate
#include <memory> // Added
#include <thread> // Added for concurrency
#include <atomic> // Added
#include "pome_ast.h" // For Program definition

namespace Pome
{
    class Chunk; // Forward declaration
    class Statement;

    /**
     * Object types
     */
    enum class ObjectType
    {
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
        NATIVE_OBJECT // Added for native wrapper objects
    };

    /**
     * Base Object Class
     */
    class PomeObject
    {
    public:
        virtual ~PomeObject() = default;
        virtual ObjectType type() const = 0;
        virtual std::string toString() const = 0;

        /**
         * GC support
         */
        bool isMarked = false;
        uint8_t generation = 0; // 0 = Young, 1 = Old
        uint8_t age = 0;        // Age in young generation
        size_t gcSize = 0;      // Size of the object for GC accounting
        PomeObject* next = nullptr;

        virtual void markChildren(class GarbageCollector& gc) {} // Mark children for GC
    };

    class PomeString;
    class PomeFunction;
    class NativeFunction;
    class PomeList;
    class PomeTable;
    class PomeClass;
    class PomeInstance;
    class PomeModule;

    class PomeValue
    {
    public:
        PomeValue(); // Nil
        explicit PomeValue(std::monostate); // Nil
        explicit PomeValue(bool b);
        explicit PomeValue(double d);
        explicit PomeValue(PomeObject* obj);

        /**
         * Type checking
         */
        inline bool isNil() const { return value_ == (QNAN | TAG_NIL); }
        inline bool isBool() const { return value_ == (QNAN | TAG_TRUE) || value_ == (QNAN | TAG_FALSE); }
        inline bool isNumber() const { return (value_ & QNAN) != QNAN; }
        inline bool isObject() const { return (value_ & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT); }
        
        // These require full definitions or PomeObject type visibility if checking obj->type()
        // PomeObject is forward declared but defined above, so it's visible.
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

        /**
         * Getters
         */
        inline bool asBool() const {
            if (value_ == (QNAN | TAG_TRUE) || value_ == (QNAN | TAG_FALSE))
                return value_ == (QNAN | TAG_TRUE);
            if (isNumber())
                return asNumber() != 0.0;
            if (value_ == (QNAN | TAG_NIL))
                return false;
            return true; // Objects are true
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

        std::string toString() const;
        bool operator==(const PomeValue &other) const;
        bool operator!=(const PomeValue &other) const { return !(*this == other); }
        bool operator<(const PomeValue &other) const;

        PomeValue deepCopy(GarbageCollector& targetGC, std::map<PomeObject*, PomeObject*>& copiedObjects) const;

        void mark(class GarbageCollector& gc) const;

        size_t hash() const;

        // Public for internal use if needed, but try to use constructors
        uint64_t getRaw() const { return value_; }

    private:
        uint64_t value_;

        // NaN Boxing Constants
        static constexpr uint64_t QNAN = 0x7ffc000000000000;
        static constexpr uint64_t SIGN_BIT = 0x8000000000000000;

        static constexpr uint64_t TAG_NIL = 1;
        static constexpr uint64_t TAG_FALSE = 2;
        static constexpr uint64_t TAG_TRUE = 3;
    };

    /**
     * String Object
     */
    class PomeString : public PomeObject
    {
    public:
        explicit PomeString(std::string value) : value_(std::move(value)) {}
        ObjectType type() const override { return ObjectType::STRING; }
        std::string toString() const override { return value_; }
        const std::string &getValue() const { return value_; }
        size_t extraSize() const { return value_.capacity(); }

    private:
        std::string value_;
    };

    /**
     * User-defined Function Object
     */
    class PomeFunction : public PomeObject
    {
    public:
        PomeFunction(); // Need constructor to init chunk
        std::string name;
        std::vector<std::string> parameters;
        const std::vector<std::unique_ptr<Statement>> *body = nullptr; // AST body
        std::shared_ptr<Chunk> chunk; // Compiled bytecode
        std::vector<PomeValue> upvalues; // Captured variables
        uint16_t upvalueCount = 0; // Number of upvalues to capture
        bool isAsync = false; // Whether this is an async function
        class PomeModule* module = nullptr; // Parent module
        class PomeClass* klass = nullptr; // Parent class (if method)

        ObjectType type() const override { return ObjectType::FUNCTION; }
        std::string toString() const override { return "<fn " + name + ">"; }
        void markChildren(class GarbageCollector& gc) override;
    };

    /**
     * Built-in Function Object
     */
    using NativeFn = std::function<PomeValue(const std::vector<PomeValue> &)>;

    class NativeFunction : public PomeObject
    {
    public:
        explicit NativeFunction(std::string name, NativeFn func)
            : name_(std::move(name)), function_(std::move(func)) {}

        ObjectType type() const override { return ObjectType::NATIVE_FUNCTION; }
        std::string toString() const override { return "<native fn " + name_ + ">"; }

        PomeValue call(const std::vector<PomeValue> &args);
        const std::string &getName() const { return name_; }

    private:
        std::string name_;
        NativeFn function_;
    };

    /**
     * List Object
     */
    class PomeList : public PomeObject
    {
    public:
        std::vector<PomeValue> elements;

        PomeList() = default;
        explicit PomeList(std::vector<PomeValue> elems) : elements(std::move(elems)) {}
        ObjectType type() const override { return ObjectType::LIST; }
        std::string toString() const override;
        void markChildren(class GarbageCollector& gc) override;
        size_t extraSize() const { return elements.capacity() * sizeof(PomeValue); }
    };

    /**
     * Table Object
     */
    class PomeTable : public PomeObject
    {
    public:
        std::map<PomeValue, PomeValue> elements;

        explicit PomeTable() {}
        explicit PomeTable(std::map<PomeValue, PomeValue> elems) : elements(std::move(elems)) {}
        ObjectType type() const override { return ObjectType::TABLE; }
        std::string toString() const override;
        void markChildren(class GarbageCollector& gc) override;
    };

    /**
     * Class Definition Object
     */
    class PomeClass : public PomeObject
    {
    public:
        std::string name;
        class PomeClass* superclass = nullptr;
        std::map<std::string, PomeFunction*> methods;
        std::map<std::string, uint8_t> fieldNames; // name -> index in fieldsArray

        explicit PomeClass(std::string name) : name(std::move(name)) {}
        ObjectType type() const override { return ObjectType::CLASS; }
        std::string toString() const override { return "<class " + name + ">"; }
        void markChildren(class GarbageCollector& gc) override;

        PomeFunction* findMethod(const std::string &name)
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
    };

    /**
     * Class Instance Object
     */
    class PomeInstance : public PomeObject
    {
    public:
        PomeClass* klass;
        std::map<std::string, PomeValue> fields;
        
        // Fast fields
        PomeValue* fieldsArray = nullptr;
        uint8_t fieldCount = 0;

        explicit PomeInstance(PomeClass* k) : klass(k) {}
        ~PomeInstance() override { if (fieldsArray) delete[] fieldsArray; }
        ObjectType type() const override { return ObjectType::INSTANCE; }
        std::string toString() const override { return "<instance of " + klass->name + ">"; }
        void markChildren(class GarbageCollector& gc) override;

        PomeValue get(const std::string &name);
        void set(const std::string &name, PomeValue value);
        void setFieldsArray(class GarbageCollector& gc, uint8_t count);
    };

    /**
     * Module Object
     */
    class PomeModule : public PomeObject
    {
    public:
        std::string scriptPath;
        std::map<PomeValue, PomeValue> exports;
        std::shared_ptr<Program> ast_root; // Keeps the AST alive for script-based modules

        explicit PomeModule() {}
        ObjectType type() const override { return ObjectType::MODULE; }
        std::string toString() const override { return "<module>"; }
        void markChildren(GarbageCollector& gc) override; // Declaration only
    };

    using ModuleLoader = std::function<PomeValue(const std::string&)>;

    /**
     * Thread Object (Isolate)
     */
    class PomeThread : public PomeObject
    {
    public:
        std::thread handle;
        std::atomic<bool> isFinished{false};
        PomeValue result;
        std::unique_ptr<class GarbageCollector> isolateGC; // Keep the child heap alive until joined

        explicit PomeThread();
        ~PomeThread() override;
        ObjectType type() const override { return ObjectType::THREAD; }
        std::string toString() const override { return "<thread>"; }
        void markChildren(GarbageCollector& gc) override;
    };

    class PomeTask : public PomeObject
    {
    public:
        PomeFunction* function = nullptr;
        std::vector<PomeValue> args; // Arguments for the task
        bool isCompleted = false;
        PomeValue result;

        explicit PomeTask(PomeFunction* fn) : function(fn) {}
        ObjectType type() const override { return ObjectType::TASK; }
        std::string toString() const override { return "<task>"; }
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * Native Wrapper Object
     */
    class PomeNativeObject : public PomeObject
    {
    public:
        void* ptr = nullptr;
        std::string tag; // Optional description/type name

        explicit PomeNativeObject(void* p, std::string t = "native") : ptr(p), tag(std::move(t)) {}
        ObjectType type() const override { return ObjectType::NATIVE_OBJECT; }
        std::string toString() const override { return "<native " + tag + " at " + std::to_string((uintptr_t)ptr) + ">"; }
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

#endif // POME_VALUE_H
