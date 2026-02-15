#ifndef POME_STDLIB_H
#define POME_STDLIB_H

#include "pome_value.h"
#include "pome_gc.h" // Needed for allocation

namespace Pome
{
    namespace StdLib
    {

        /**
         * Creates and returns the 'math' module
         */
        PomeModule* createMathModule(GarbageCollector& gc);

        /**
         * Creates and returns the 'io' module
         */
        PomeModule* createIOModule(GarbageCollector& gc);

        /**
         * Creates and returns the 'string' module
         */
        PomeModule* createStringModule(GarbageCollector& gc);

        /**
         * Creates and returns the 'time' module
         */
        PomeModule* createTimeModule(GarbageCollector& gc);

    }
}

#endif // POME_STDLIB_H
