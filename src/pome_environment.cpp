#include "../include/pome_environment.h"
#include "../include/pome_gc.h" // For GarbageCollector definition

namespace Pome
{

    Environment::Environment(Interpreter* interp, Environment* parent) : interpreter_(interp), parent_(parent) {}

    void Environment::define(const std::string& name, const PomeValue& value) {
        store_[name] = value;
    }

    PomeValue Environment::get(const std::string& name) {
        if (store_.count(name)) {
            return store_[name];
        }
        if (parent_) {
            return parent_->get(name);
        }
        throw std::runtime_error("Undefined variable '" + name + "'.");
    }

    void Environment::assign(const std::string& name, const PomeValue& value) {
        if (store_.count(name)) {
            store_[name] = value;
            return;
        }
        if (parent_) {
            parent_->assign(name, value);
            return;
        }
        throw std::runtime_error("Undefined variable '" + name + "'.");
    }

    void Environment::markChildren(GarbageCollector& gc) {
        if (parent_) {
            gc.markObject(parent_);
        }
        for (auto const& [name, val] : store_) { // Structured binding for C++17
            val.mark(gc);
        }
    }

} // namespace Pome