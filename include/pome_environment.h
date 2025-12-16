#ifndef POME_ENVIRONMENT_H
#define POME_ENVIRONMENT_H

#include "pome_value.h"
#include <map>
#include <string>

namespace Pome
{
    class Interpreter; // Forward declaration

    class Environment : public PomeObject
    {
    public:
        Environment(Interpreter* interp, Environment* parent = nullptr);

        ObjectType type() const override { return ObjectType::ENVIRONMENT; }
        std::string toString() const override { return "<environment>"; }

        void define(const std::string &name, const PomeValue &value);
        PomeValue get(const std::string &name);
        void assign(const std::string &name, const PomeValue &value);
        
        Environment* getParent() const { return parent_; }
        std::map<std::string, PomeValue>& getStore() { return store_; }

        void markChildren(class GarbageCollector& gc) override; // Declare markChildren

    private:
        std::map<std::string, PomeValue> store_;
        Interpreter* interpreter_; // Add interpreter pointer
        Environment* parent_;
    };

} // namespace Pome

#endif // POME_ENVIRONMENT_H