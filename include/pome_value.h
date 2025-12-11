#ifndef POME_VALUE_H
#define POME_VALUE_H

#include <string>
#include <variant>
#include <vector>
#include <map>
#include <functional>
#include <memory>
#include <iostream>

namespace Pome
{

    // Forward declarations
    class Environment;
    class Statement;

    // Object types
    enum class ObjectType
    {
        STRING,
        FUNCTION,
        NATIVE_FUNCTION,
        LIST,
        TABLE,
        CLASS,
        INSTANCE,
        MODULE
    };

    // Base Object Class
    class PomeObject
    {
    public:
        virtual ~PomeObject() = default;
        virtual ObjectType type() const = 0;
        virtual std::string toString() const = 0;
    };

    class PomeString;
    class PomeFunction;
    class NativeFunction;
    class PomeList;
    class PomeTable;
    class PomeClass;
    class PomeInstance;
    class PomeModule;

    using PomeValueType = std::variant<
        std::monostate,
        bool,
        double,
        std::shared_ptr<PomeObject>>;

    class PomeValue
    {
    public:
        PomeValue();
        explicit PomeValue(std::monostate);
        explicit PomeValue(bool b);
        explicit PomeValue(double d);
        explicit PomeValue(std::shared_ptr<PomeObject> obj);
        explicit PomeValue(const std::string &s); // Helper for string
        explicit PomeValue(const char *s);        // Helper for C-string

        // Type checking
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
        bool isEnvironment() const;

        // Getters
        bool asBool() const;
        double asNumber() const;
        const std::string &asString() const;
        std::shared_ptr<PomeFunction> asPomeFunction() const;
        std::shared_ptr<NativeFunction> asNativeFunction() const;
        std::shared_ptr<PomeList> asList() const;
        std::shared_ptr<PomeTable> asTable() const;
        std::shared_ptr<PomeClass> asClass() const;
        std::shared_ptr<PomeInstance> asInstance() const;
        std::shared_ptr<PomeModule> asModule() const;

        std::string toString() const;
        bool operator==(const PomeValue &other) const;
        bool operator!=(const PomeValue &other) const { return !(*this == other); }
        bool operator<(const PomeValue &other) const;

    private:
        PomeValueType value_;
    };

    // String Object
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

    // User-defined Function Object
    class PomeFunction : public PomeObject
    {
    public:
        std::string name;
        std::vector<std::string> parameters;
        const std::vector<std::unique_ptr<Statement>> *body;
        std::shared_ptr<Environment> closureEnv;
        std::shared_ptr<PomeFunction> bind(std::shared_ptr<PomeInstance> instance);

        ObjectType type() const override { return ObjectType::FUNCTION; }
        std::string toString() const override { return "<fn " + name + ">"; }
    };

    // Built-in Function Object
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

    // List Object
    class PomeList : public PomeObject
    {
    public:
        std::vector<PomeValue> elements;

        explicit PomeList(std::vector<PomeValue> elems) : elements(std::move(elems)) {}
        ObjectType type() const override { return ObjectType::LIST; }
        std::string toString() const override
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
    };

    // Table Object
    class PomeTable : public PomeObject
    {
    public:
        std::map<PomeValue, PomeValue> elements;

        explicit PomeTable(std::map<PomeValue, PomeValue> elems) : elements(std::move(elems)) {}
        ObjectType type() const override { return ObjectType::TABLE; }
        std::string toString() const override
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
    };

    // Class Definition Object
    class PomeClass : public PomeObject
    {
    public:
        std::string name;
        std::map<std::string, std::shared_ptr<PomeFunction>> methods;

        explicit PomeClass(std::string n) : name(std::move(n)) {}
        ObjectType type() const override { return ObjectType::CLASS; }
        std::string toString() const override { return "<class " + name + ">"; }

        std::shared_ptr<PomeFunction> findMethod(const std::string &name)
        {
            auto it = methods.find(name);
            if (it != methods.end())
            {
                return it->second;
            }
            return nullptr;
        }
    };

    // Class Instance Object
    class PomeInstance : public PomeObject
    {
    public:
        std::shared_ptr<PomeClass> klass;
        std::map<std::string, PomeValue> fields;

        explicit PomeInstance(std::shared_ptr<PomeClass> k) : klass(std::move(k)) {}
        ObjectType type() const override { return ObjectType::INSTANCE; }
        std::string toString() const override { return "<instance of " + klass->name + ">"; }

        PomeValue get(const std::string &name)
        {
            auto it = fields.find(name);
            if (it != fields.end())
            {
                std::cout << "Getting field " << name << " on " << this << ": " << it->second.toString() << std::endl;
                return it->second;
            }
            std::cout << "Getting field " << name << " on " << this << ": nil (not found)" << std::endl;
            return PomeValue(); // nil
        }

        void set(const std::string &name, PomeValue value)
        {
            std::cout << "Setting field " << name << " on " << this << " to " << value.toString() << std::endl;
            fields[name] = value;
        }
    };

    // Module Object (Wraps Environment)
    class PomeModule : public PomeObject
    {
    public:
        std::shared_ptr<std::map<PomeValue, PomeValue>> exports;

        explicit PomeModule(std::shared_ptr<std::map<PomeValue, PomeValue>> exportsMap) : exports(std::move(exportsMap)) {}
        ObjectType type() const override { return ObjectType::MODULE; }
        std::string toString() const override { return "<module>"; }
    };

} // namespace Pome

#endif // POME_VALUE_H
