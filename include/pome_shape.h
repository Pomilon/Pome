#ifndef POME_SHAPE_H
#define POME_SHAPE_H

#include "pome_base.h"
#include <unordered_map>
#include <string>

namespace Pome {

    class PomeShape : public PomeObject {
    public:
        PomeShape* parent;
        PomeValue propertyKey;
        int propertyIndex;
        std::unordered_map<PomeValue, PomeShape*> transitions;

        PomeShape(PomeShape* parent, PomeValue key, int index)
            : parent(parent), propertyKey(key), propertyIndex(index) {}

        ObjectType type() const override { return ObjectType::SHAPE; }
        std::string toString() const override { return "<shape>"; }
        void markChildren(GarbageCollector& gc) override;

        PomeShape* transition(GarbageCollector& gc, PomeValue key);
        int getIndex(PomeValue key);
    };

} // namespace Pome

#endif
