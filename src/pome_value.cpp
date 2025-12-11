#include "../include/pome_value.h"
#include "../include/pome_environment.h"
#include <sstream>
#include <iomanip>

namespace Pome
{

    // --- NativeFunction Implementation ---
    PomeValue NativeFunction::call(const std::vector<PomeValue> &args)
    {
        return function_(args);
    }

    // --- PomeValue Implementation ---

    PomeValue::PomeValue() : value_(std::monostate{}) {}
    PomeValue::PomeValue(std::monostate) : value_(std::monostate{}) {}
    PomeValue::PomeValue(bool b) : value_(b) {}
    PomeValue::PomeValue(double d) : value_(d) {}
    PomeValue::PomeValue(std::shared_ptr<PomeObject> obj) : value_(std::move(obj)) {}
    PomeValue::PomeValue(const std::string &s) : value_(std::make_shared<PomeString>(s)) {}
    PomeValue::PomeValue(const char *s) : value_(std::make_shared<PomeString>(s)) {}

    // Type checking
    bool PomeValue::isNil() const { return std::holds_alternative<std::monostate>(value_); }
    bool PomeValue::isBool() const { return std::holds_alternative<bool>(value_); }
    bool PomeValue::isNumber() const { return std::holds_alternative<double>(value_); }

    bool PomeValue::isString() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::STRING;
        }
        return false;
    }

    bool PomeValue::isFunction() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::FUNCTION || (*obj)->type() == ObjectType::NATIVE_FUNCTION;
        }
        return false;
    }

    bool PomeValue::isPomeFunction() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::FUNCTION;
        }
        return false;
    }

    bool PomeValue::isNativeFunction() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::NATIVE_FUNCTION;
        }
        return false;
    }

    bool PomeValue::isList() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::LIST;
        }
        return false;
    }

    bool PomeValue::isTable() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::TABLE;
        }
        return false;
    }

    bool PomeValue::isClass() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::CLASS;
        }
        return false;
    }

    bool PomeValue::isInstance() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::INSTANCE;
        }
        return false;
    }

    bool PomeValue::isEnvironment() const
    {
        if (auto obj = std::get_if<std::shared_ptr<PomeObject>>(&value_))
        {
            return (*obj)->type() == ObjectType::MODULE; // We use Module as Environment-like object
        }
        return false;
    }

    // Getters
    bool PomeValue::asBool() const
    {
        if (isBool())
            return std::get<bool>(value_);
        if (isNumber())
            return std::get<double>(value_) != 0.0;
        if (isNil())
            return false;
        return true;
    }

    double PomeValue::asNumber() const
    {
        return std::get<double>(value_);
    }

    const std::string &PomeValue::asString() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeString>(obj)->getValue();
    }

    std::shared_ptr<PomeFunction> PomeValue::asPomeFunction() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeFunction>(obj);
    }

    std::shared_ptr<PomeFunction> PomeFunction::bind(std::shared_ptr<PomeInstance> instance)
    {
        // 1. Create a new environment using the function's closure as the parent.
        auto environment = std::make_shared<Environment>(closureEnv);

        // 2. Define "this" in that environment, pointing to the instance.
        environment->define("this", PomeValue(instance));

        // 3. Create a new function that is exactly the same, but uses this new environment.
        auto boundMethod = std::make_shared<PomeFunction>();
        boundMethod->name = name;
        boundMethod->parameters = parameters;
        boundMethod->body = body;
        boundMethod->closureEnv = environment; // The magic fix

        return boundMethod;
    }

    std::shared_ptr<NativeFunction> PomeValue::asNativeFunction() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<NativeFunction>(obj);
    }

    std::shared_ptr<PomeList> PomeValue::asList() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeList>(obj);
    }

    std::shared_ptr<PomeTable> PomeValue::asTable() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeTable>(obj);
    }

    std::shared_ptr<PomeClass> PomeValue::asClass() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeClass>(obj);
    }

    std::shared_ptr<PomeInstance> PomeValue::asInstance() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeInstance>(obj);
    }

    std::shared_ptr<PomeModule> PomeValue::asModule() const
    {
        auto obj = std::get<std::shared_ptr<PomeObject>>(value_);
        return std::static_pointer_cast<PomeModule>(obj);
    }

    std::string PomeValue::toString() const
    {
        if (isNil())
            return "nil";
        if (isBool())
            return std::get<bool>(value_) ? "true" : "false";
        if (isNumber())
        {
            std::stringstream ss;
            double d = std::get<double>(value_);
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
        if (std::holds_alternative<std::shared_ptr<PomeObject>>(value_))
        {
            return std::get<std::shared_ptr<PomeObject>>(value_)->toString();
        }
        return "unknown";
    }

    bool PomeValue::operator==(const PomeValue &other) const
    {
        if (value_.index() != other.value_.index())
            return false;
        if (isNil())
            return true;
        if (isBool())
            return std::get<bool>(value_) == std::get<bool>(other.value_);
        if (isNumber())
            return std::get<double>(value_) == std::get<double>(other.value_);

        auto obj1 = std::get<std::shared_ptr<PomeObject>>(value_);
        auto obj2 = std::get<std::shared_ptr<PomeObject>>(other.value_);

        if (obj1 == obj2)
            return true;

        if (obj1->type() == ObjectType::STRING && obj2->type() == ObjectType::STRING)
        {
            return std::static_pointer_cast<PomeString>(obj1)->getValue() ==
                   std::static_pointer_cast<PomeString>(obj2)->getValue();
        }
        return false;
    }

    bool PomeValue::operator<(const PomeValue &other) const
    {
        if (value_.index() != other.value_.index())
            return value_.index() < other.value_.index();

        if (isBool())
            return std::get<bool>(value_) < std::get<bool>(other.value_);
        if (isNumber())
            return std::get<double>(value_) < std::get<double>(other.value_);

        if (std::holds_alternative<std::shared_ptr<PomeObject>>(value_))
        {
            auto obj1 = std::get<std::shared_ptr<PomeObject>>(value_);
            auto obj2 = std::get<std::shared_ptr<PomeObject>>(other.value_);

            // Strict ordering by ObjectType first
            if (obj1->type() != obj2->type())
            {
                return obj1->type() < obj2->type();
            }

            // Same type comparison
            if (obj1->type() == ObjectType::STRING)
            {
                return std::static_pointer_cast<PomeString>(obj1)->getValue() <
                       std::static_pointer_cast<PomeString>(obj2)->getValue();
            }
            // Fallback to pointer comparison for other reference types (functions, lists, etc.)
            return obj1 < obj2;
        }
        return false; // nil == nil
    }

} // namespace
