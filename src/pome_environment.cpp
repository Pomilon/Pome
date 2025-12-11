#include "../include/pome_environment.h"
#include <stdexcept>

namespace Pome
{

    Environment::Environment() : parent_(nullptr) {}

    Environment::Environment(std::shared_ptr<Environment> parent) : parent_(std::move(parent)) {}

    void Environment::define(const std::string &name, const PomeValue &value)
    {
        // In Pome, re-defining an existing variable in the current scope should overwrite it
        // or be an error, depending on language semantics. For simplicity, we'll allow overwrite.
        store_[name] = value;
    }

    PomeValue Environment::get(const std::string &name)
    {
        if (store_.count(name))
        {
            std::cerr << "DEBUG: Environment::get found '" << name << "' in current scope." << std::endl;
            return store_[name];
        }
        if (parent_)
        {
            std::cerr << "DEBUG: Environment::get looking for '" << name << "' in parent scope." << std::endl;
            return parent_->get(name);
        }
        std::cerr << "DEBUG: Environment::get could not find '" << name << "', throwing error." << std::endl;
        throw std::runtime_error("Undefined variable: " + name);
    }

    void Environment::assign(const std::string &name, const PomeValue &value)
    {
        if (store_.count(name))
        {
            store_[name] = value;
            return;
        }
        if (parent_)
        {
            parent_->assign(name, value);
            return;
        }
        throw std::runtime_error("Cannot assign to undefined variable: " + name);
    }

} // namespace Pome
