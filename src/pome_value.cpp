#include "../include/pome_value.h"
#include "../include/pome_environment.h"
#include <sstream>
#include <iomanip>

namespace Pome
{

    /**
     * --- NativeFunction Implementation ---
     */
    PomeValue NativeFunction::call(const std::vector<PomeValue> &args)
    {
        return function_(args);
    }

    /**
     * --- PomeValue Implementation ---
     */

    PomeValue::PomeValue() : value_(std::monostate{}) {}
    PomeValue::PomeValue(std::monostate) : value_(std::monostate{}) {}
    PomeValue::PomeValue(bool b) : value_(b) {}
    PomeValue::PomeValue(double d) : value_(d) {}
    PomeValue::PomeValue(PomeObject* obj) : value_(obj) {}

    /**
     * Type checking
     */
    bool PomeValue::isNil() const { return std::holds_alternative<std::monostate>(value_); }
    bool PomeValue::isBool() const { return std::holds_alternative<bool>(value_); }
    bool PomeValue::isNumber() const { return std::holds_alternative<double>(value_); }

    bool PomeValue::isString() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::STRING;
        }
        return false;
    }

    bool PomeValue::isFunction() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::FUNCTION || (*obj)->type() == ObjectType::NATIVE_FUNCTION;
        }
        return false;
    }

    bool PomeValue::isPomeFunction() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::FUNCTION;
        }
        return false;
    }

    bool PomeValue::isNativeFunction() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::NATIVE_FUNCTION;
        }
        return false;
    }

    bool PomeValue::isList() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::LIST;
        }
        return false;
    }

    bool PomeValue::isTable() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::TABLE;
        }
        return false;
    }

    bool PomeValue::isClass() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::CLASS;
        }
        return false;
    }

    bool PomeValue::isInstance() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::INSTANCE;
        }
        return false;
    }

    bool PomeValue::isModule() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::MODULE; 
        }
        return false;
    }

    bool PomeValue::isEnvironment() const
    {
        if (auto obj = std::get_if<PomeObject*>(&value_))
        {
            return (*obj)->type() == ObjectType::ENVIRONMENT; 
        }
        return false;
    }

    /**
     * Getters
     */
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
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeString*>(obj)->getValue();
    }

    PomeObject* PomeValue::asObject() const 
    {
        if (std::holds_alternative<PomeObject*>(value_))
            return std::get<PomeObject*>(value_);
        return nullptr;
    }

    PomeFunction* PomeValue::asPomeFunction() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeFunction*>(obj);
    }

    NativeFunction* PomeValue::asNativeFunction() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<NativeFunction*>(obj);
    }

    PomeList* PomeValue::asList() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeList*>(obj);
    }

    PomeTable* PomeValue::asTable() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeTable*>(obj);
    }

    PomeClass* PomeValue::asClass() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeClass*>(obj);
    }

    PomeInstance* PomeValue::asInstance() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeInstance*>(obj);
    }

    PomeModule* PomeValue::asModule() const
    {
        if (!isModule()) return nullptr;
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<PomeModule*>(obj);
    }

    Environment* PomeValue::asEnvironment() const
    {
        auto obj = std::get<PomeObject*>(value_);
        return static_cast<Environment*>(obj);
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
        if (std::holds_alternative<PomeObject*>(value_))
        {
            return std::get<PomeObject*>(value_)->toString();
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

        auto obj1 = std::get<PomeObject*>(value_);
        auto obj2 = std::get<PomeObject*>(other.value_);

        if (obj1 == obj2)
            return true;
            
        if (obj1 == nullptr || obj2 == nullptr)
            return false;

        if (obj1->type() == ObjectType::STRING && obj2->type() == ObjectType::STRING)
        {
            return static_cast<PomeString*>(obj1)->getValue() ==
                   static_cast<PomeString*>(obj2)->getValue();
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

        if (std::holds_alternative<PomeObject*>(value_))
        {
            auto obj1 = std::get<PomeObject*>(value_);
            auto obj2 = std::get<PomeObject*>(other.value_);

            /**
             * Strict ordering by ObjectType first
             */
            if (obj1->type() != obj2->type())
            {
                return obj1->type() < obj2->type();
            }

            /**
             * Same type comparison
             */
            if (obj1->type() == ObjectType::STRING)
            {
                return static_cast<PomeString*>(obj1)->getValue() <
                       static_cast<PomeString*>(obj2)->getValue();
            }
            return obj1 < obj2;
        }
        return false; // nil == nil
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
    
    /**
     * PomeInstance implementation
     */
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

} // namespace
