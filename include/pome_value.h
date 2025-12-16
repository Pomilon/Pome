#ifndef POME_VALUE_H
#define POME_VALUE_H

#include <string>
#include <variant>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <iostream>
#include "pome_ast.h" // For Program definition

namespace Pome
{

    /**
     * Forward declarations
     */
    class Environment;
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
        ENVIRONMENT,
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
        size_t gcSize = 0; // Size of the object for GC accounting
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

    /**
     * Modified to use raw pointer
     */
    using PomeValueType = std::variant<
        std::monostate,
        bool,
        double,
        PomeObject*>;

    class PomeValue
    {
    public:
        PomeValue();
        explicit PomeValue(std::monostate);
        explicit PomeValue(bool b);
        explicit PomeValue(double d);
        explicit PomeValue(PomeObject* obj);

        /**
         * Type checking
         */
        bool isNil() const;
        bool isBool() const;
        bool isNumber() const;
        bool isString() const;
        bool isFunction() const;
        bool isPomeFunction() const;
        bool isNativeFunction() const;
        bool isList() const;
        bool isTable() const;
        bool isClass() const;
        bool isInstance() const;
        bool isModule() const;
        bool isEnvironment() const;
        bool isObject() const; // Added for convenience in GC marking

        /**
         * Getters
         */
        bool asBool() const;
        double asNumber() const;
        const std::string &asString() const;
        PomeObject* asObject() const;
        PomeFunction* asPomeFunction() const;
        NativeFunction* asNativeFunction() const;
        PomeList* asList() const;
        PomeTable* asTable() const;
        PomeClass* asClass() const;
        PomeInstance* asInstance() const;
        PomeModule* asModule() const;
        Environment* asEnvironment() const; // Added

        std::string toString() const;
        bool operator==(const PomeValue &other) const;
        bool operator!=(const PomeValue &other) const { return !(*this == other); }
        bool operator<(const PomeValue &other) const;

        void mark(class GarbageCollector& gc) const; // Mark contained object for GC traversal

    private:
        PomeValueType value_;
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

    private:
        std::string value_;
    };

    /**
     * User-defined Function Object
     */
    class PomeFunction : public PomeObject
    {
    public:
        std::string name;
        std::vector<std::string> parameters;
        const std::vector<std::unique_ptr<Statement>> *body; // Pointer to AST body
        Environment* closureEnv = nullptr; 

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

        explicit PomeList(std::vector<PomeValue> elems) : elements(std::move(elems)) {}
        ObjectType type() const override { return ObjectType::LIST; }
        std::string toString() const override;
        void markChildren(class GarbageCollector& gc) override;
    };

    /**
     * Table Object
     */
    class PomeTable : public PomeObject
    {
    public:
        std::map<PomeValue, PomeValue> elements;

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
        std::map<std::string, PomeFunction*> methods;

        explicit PomeClass(std::string n) : name(std::move(n)) {}
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

        explicit PomeInstance(PomeClass* k) : klass(k) {}
        ObjectType type() const override { return ObjectType::INSTANCE; }
        std::string toString() const override { return "<instance of " + klass->name + ">"; }
        void markChildren(class GarbageCollector& gc) override;

        PomeValue get(const std::string &name);
        void set(const std::string &name, PomeValue value);
    };

    /**
     * Module Object
     */
    class PomeModule : public PomeObject
    {
    public:
        std::map<PomeValue, PomeValue> exports;
        std::shared_ptr<Program> ast_root; // Keeps the AST alive for script-based modules

        explicit PomeModule() {}
        ObjectType type() const override { return ObjectType::MODULE; }
        std::string toString() const override { return "<module>"; }
        void markChildren(GarbageCollector& gc) override; // Declaration only
    };

} // namespace Pome

#endif // POME_VALUE_H
