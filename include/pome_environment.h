#ifndef POME_ENVIRONMENT_H
#define POME_ENVIRONMENT_H

#include "pome_value.h"
#include <map>
#include <string>

namespace Pome
{

    
    class Environment : public PomeObject
    {
    public:
        Environment();
        /**
         * Constructor for a nested environment
         */
        explicit Environment(Environment* parent);

        ObjectType type() const override { return ObjectType::ENVIRONMENT; }
        std::string toString() const override { return "<environment>"; }

        /**
         * Define a new variable in the current scope
         */
        void define(const std::string &name, const PomeValue &value);

        /**
         * Get the value of a variable, searching parent scopes if necessary
         */
        PomeValue get(const std::string &name);

        /**
         * Assign a new value to an existing variable, searching parent scopes
         */
        void assign(const std::string &name, const PomeValue &value);
        
        /**
         * For GC tracing
         */
        Environment* getParent() const { return parent_; }
        std::map<std::string, PomeValue>& getStore() { return store_; }

        // GC Mark Children
        void markChildren(class GarbageCollector& gc) override; // Declare markChildren

    private:
        std::map<std::string, PomeValue> store_;
        Environment* parent_; // Pointer to the enclosing scope
    };

} // namespace Pome

#endif // POME_ENVIRONMENT_H
