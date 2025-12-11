#ifndef POME_STDLIB_H
#define POME_STDLIB_H

#include "pome_value.h"
#include <map>
#include <memory>

namespace Pome
{
    namespace StdLib
    {

        // Creates and returns the exports map for the 'math' module
        std::shared_ptr<std::map<PomeValue, PomeValue>> createMathExports();

        // Creates and returns the exports map for the 'io' module
        std::shared_ptr<std::map<PomeValue, PomeValue>> createIOExports();

        // Creates and returns the exports map for the 'string' module
        // (Can be used as a standalone module or methods on string objects later)
        std::shared_ptr<std::map<PomeValue, PomeValue>> createStringExports();

    }
}

#endif // POME_STDLIB_H
