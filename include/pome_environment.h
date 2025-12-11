#ifndef POME_ENVIRONMENT_H
#define POME_ENVIRONMENT_H

#include "pome_value.h"
#include <map>
#include <string>
#include <memory> // For std::shared_ptr

namespace Pome
{

    class Environment
    {
    public:
        // Constructor for the global environment (no parent)
        Environment();
        // Constructor for a nested environment
        explicit Environment(std::shared_ptr<Environment> parent);

        // Define a new variable in the current scope
        void define(const std::string &name, const PomeValue &value);

        // Get the value of a variable, searching parent scopes if necessary
        PomeValue get(const std::string &name);

        // Assign a new value to an existing variable, searching parent scopes
        void assign(const std::string &name, const PomeValue &value);

    private:
        std::map<std::string, PomeValue> store_;
        std::shared_ptr<Environment> parent_; // Pointer to the enclosing scope
    };

} // namespace Pome

#endif // POME_ENVIRONMENT_H
