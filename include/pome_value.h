#ifndef POME_VALUE_H
#define POME_VALUE_H

#include "pome_gc.h"
#include <unordered_map>

namespace Pome
{
    /**
     * String Object
     */
    class PomeString : public PomeObject
    {
    public:
        explicit PomeString(std::string value);
        ObjectType type() const override { return ObjectType::STRING; }
        std::string toString() const override { return value_; }
        const std::string &getValue() const { return value_; }
        size_t extraSize() const { return value_.capacity(); }
        size_t getHash() const { return hash_; }

    private:
        std::string value_;
        size_t hash_;
    };

    /**
     * Upvalue Object
     */
    class PomeUpvalue : public PomeObject
    {
    public:
        PomeValue* location; 
        PomeValue closedValue;
        PomeUpvalue* next = nullptr; 

        PomeUpvalue(PomeValue* slot) : location(slot), closedValue() {}
        ObjectType type() const override { return ObjectType::UPVALUE; }
        std::string toString() const override { return "<upvalue>"; }
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * User-defined Function Object
     */
    class PomeFunction : public PomeObject
    {
    public:
        PomeFunction(); 
        std::string name;
        std::vector<std::string> parameters;
        const std::vector<std::unique_ptr<Statement>> *body = nullptr; 
        std::shared_ptr<Chunk> chunk; 
        std::vector<PomeUpvalue*> upvalues; 
        uint16_t upvalueCount = 0; 
        bool isAsync = false; 
        class PomeModule* module = nullptr; 
        class PomeClass* klass = nullptr; 

        ObjectType type() const override { return ObjectType::FUNCTION; }
        std::string toString() const override { return "<fn " + name + ">"; }
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * Built-in Function Object
     */
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
    enum class ListType : uint8_t {
        MIXED,
        DOUBLE,
        INT32
    };

    /**
     * List Object
     */
    class PomeList : public PomeObject
    {
    public:
        std::vector<PomeValue> elements;
        void* unboxedData = nullptr;
        size_t unboxedCount = 0;
        size_t unboxedCapacity = 0;
        ListType listType = ListType::MIXED;

        PomeList();
        ~PomeList() override;
        explicit PomeList(std::vector<PomeValue> elems);
        
        ObjectType type() const override { return ObjectType::LIST; }
        std::string toString() const override;
        void markChildren(GarbageCollector& gc) override;
        size_t extraSize() const;

        bool isUnboxed() const { return listType != ListType::MIXED; }
        void tryUnbox();
        void box();
        void ensureCapacity(size_t capacity);
        void push(GarbageCollector& gc, PomeValue val);
        
        void switchTo(ListType newType);
        
        // Inline accessors for speed
        double* asDouble() const { return (double*)unboxedData; }
        int32_t* asInt32() const { return (int32_t*)unboxedData; }
    };

    /**
     * Table Object
     */
    class PomeTable : public PomeObject
    {
    public:
        PomeShape* shape;
        std::vector<PomeValue> properties;
        std::unordered_map<PomeValue, PomeValue> backfill; 

        PomeTable(PomeShape* s);

        void set(GarbageCollector& gc, PomeValue key, PomeValue value);
        PomeValue get(PomeValue key);

        ObjectType type() const override { return ObjectType::TABLE; }
        std::string toString() const override;
        void markChildren(GarbageCollector& gc) override;
        std::vector<PomeValue> getSortedKeys() const;
        size_t extraSize() const {
            return properties.capacity() * sizeof(PomeValue) + 
                   backfill.size() * (sizeof(PomeValue) * 2 + 16); // 16 bytes overhead for hash map entry
        }
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
        std::map<std::string, uint8_t> fieldNames; 
        class PomeModule* module = nullptr; 
        PomeShape* classShape = nullptr;

        explicit PomeClass(std::string name) : name(std::move(name)) {}
        ObjectType type() const override { return ObjectType::CLASS; }
        std::string toString() const override { return "<class " + name + ">"; }
        void markChildren(GarbageCollector& gc) override;

        PomeFunction* findMethod(const std::string &name);
    };

    /**
     * Class Instance Object
     */
    class PomeInstance : public PomeObject
    {
    public:
        PomeClass* klass;
        PomeShape* shape;
        std::vector<PomeValue> properties;

        explicit PomeInstance(PomeClass* k, PomeShape* s);
        ObjectType type() const override { return ObjectType::INSTANCE; }
        std::string toString() const override { return "<instance of " + klass->name + ">"; }

        void set(GarbageCollector& gc, PomeValue key, PomeValue value);
        PomeValue get(PomeValue key);
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * Module Object
     */
    class PomeModule : public PomeObject
    {
    public:
        std::string scriptPath;
        std::unordered_map<PomeValue, PomeValue> exports;
        std::unordered_map<PomeValue, PomeValue> variables;

        ObjectType type() const override { return ObjectType::MODULE; }
        std::string toString() const override { return "<module " + scriptPath + ">"; }
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * Thread Object
     */
    class PomeThread : public PomeObject
    {
    public:
        std::thread handle;
        PomeValue result;
        std::atomic<bool> isFinished{false};
        std::unique_ptr<GarbageCollector> isolateGC;

        ObjectType type() const override { return ObjectType::THREAD; }
        std::string toString() const override { return "<thread>"; }
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * Task Object
     */
    class PomeTask : public PomeObject
    {
    public:
        PomeFunction* function;
        std::vector<PomeValue> args;
        PomeValue result;
        bool isCompleted = false;

        explicit PomeTask(PomeFunction* f) : function(f) {}
        ObjectType type() const override { return ObjectType::TASK; }
        std::string toString() const override { return "<task>"; }
        void markChildren(GarbageCollector& gc) override;
    };

    /**
     * Native wrapper object
     */
    class PomeNativeObject : public PomeObject {
    public:
        void* ptr;
        std::string typeName;
        std::function<void(void*)> deleter;

        PomeNativeObject(void* p, std::string t, std::function<void(void*)> d = nullptr)
            : ptr(p), typeName(std::move(t)), deleter(std::move(d)) {}
        
        ~PomeNativeObject() override { if (deleter && ptr) deleter(ptr); }

        ObjectType type() const override { return ObjectType::NATIVE_OBJECT; }
        std::string toString() const override { return "<native " + typeName + ">"; }
    };

} // namespace Pome

#include "pome_gc_impl.h"

#endif // POME_VALUE_H
