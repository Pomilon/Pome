#include "../include/pome_environment.h"
#include "../include/pome_gc.h" // For GarbageCollector definition
#include <stdexcept>
#include <iostream>

namespace Pome
{

    Environment::Environment() : parent_(nullptr) {}

    Environment::Environment(Environment* parent) : parent_(parent) {}

    void Environment::define(const std::string &name, const PomeValue &value)
    {
        /**
         * In Pome, re-defining an existing variable in the current scope should overwrite it
         * or be an error, depending on language semantics. For simplicity, we'll allow overwrite.
         */
        store_[name] = value;
    }

    PomeValue Environment::get(const std::string &name)
    {
        if (store_.count(name))
        {
            return store_[name];
        }
        if (parent_)
        {
            return parent_->get(name);
        }
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

    void Environment::markChildren(GarbageCollector& gc) {
        if (parent_) {
            gc.markObject(parent_); // Mark parent environment
        }
        for (auto const& [name, val] : store_) {
            val.mark(gc); // Mark all stored values (which are PomeValue)
        }
    }

} // namespace Pome
