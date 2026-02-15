#include "../../../include/pome_vm.h"
#include "../../../include/pome_value.h"
#include <iostream>
#include <vector>

using namespace Pome;

// A simple native function: add_native(a, b)
PomeValue nativeAdd(const std::vector<PomeValue> &args)
{
    if (args.size() != 2)
    {
        std::cerr << "native_add expects 2 arguments" << std::endl;
        return PomeValue(std::monostate{});
    }

    // For robustness, check types, but for this test we assume numbers
    if (args[0].isNumber() && args[1].isNumber())
    {
        double result = args[0].asNumber() + args[1].asNumber();
        return PomeValue(result);
    }

    return PomeValue(std::monostate{});
}

// Entry point
// Must be extern "C" to prevent name mangling so dlsym can find it
extern "C" void pome_init(VM *vm, PomeModule *module)
{
    std::cout << "[Native Module] Initializing my_extension..." << std::endl;

    // Allocate NativeFunction object
    auto &gc = vm->getGC();

    // Create the function
    auto func = gc.allocate<NativeFunction>("add_native", nativeAdd);

    // Export it under the name "add"
    PomeString *name = gc.allocate<PomeString>("add");
    module->exports[PomeValue(name)] = PomeValue(func);
}
