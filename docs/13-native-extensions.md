# Native Extensions (C++ API)

Pome allows you to extend the language with high-performance C++ modules. This architecture is similar to Python or Node.js, where you can write performance-critical code in C++ and load it dynamically in your Pome scripts.

## Overview

A native extension is a shared library (`.so`, `.dll`, or `.dylib`) that links against `libpome`. It exposes a specific entry point function that Pome calls when you `import` the module.

## Creating an Extension

### 1. The C++ Code

Create a C++ file (e.g., `my_module.cpp`). You need to include `pome_vm.h` and define the `pome_init` entry point.

```cpp
#include "pome_vm.h"
#include "pome_value.h"
#include <iostream>
#include <vector>

using namespace Pome;

// Define your native function
PomeValue nativeAdd(const std::vector<PomeValue>& args) {
    if (args.size() != 2) {
        return PomeValue(std::monostate{}); // Return nil on error
    }
    
    if (args[0].isNumber() && args[1].isNumber()) {
        double result = args[0].asNumber() + args[1].asNumber();
        return PomeValue(result);
    }
    
    return PomeValue(std::monostate{});
}

// Entry Point
// This must be extern "C" to prevent name mangling
extern "C" void pome_init(VM* vm, PomeModule* module) {
    auto& gc = vm->getGC();
    
    // 1. Create the native function object
    auto func = gc.allocate<NativeFunction>("add", nativeAdd);
    
    // 2. Export it to the module
    PomeString* name = gc.allocate<PomeString>("add");
    module->exports[PomeValue(name)] = PomeValue(func);
    
    std::cout << "Native module loaded!" << std::endl;
}
```

### 2. Building the Extension

You need to compile this into a shared library. Here is a `CMakeLists.txt` example:

```cmake
cmake_minimum_required(VERSION 3.10)
project(my_module)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC") # Position Independent Code

# Include Pome headers
# If installed via install.sh:
include_directories(/usr/local/include/pome)
# Or point to source:
# include_directories(/path/to/Pome/include)

# Create shared library
add_library(my_module SHARED my_module.cpp)

# Link against libpome
# If installed via install.sh:
target_link_libraries(my_module /usr/local/lib/libpome.so)
# Or build dir:
# target_link_libraries(my_module /path/to/Pome/build/libpome.so)
```

### 3. Using the Extension

Once built, you will have `libmy_module.so` (Linux), `libmy_module.dylib` (macOS), or `my_module.dll` (Windows).

Place this file in your project root or a `modules/` directory.

```pome
// Loads libmy_module.so
import my_module;

var result = my_module.add(10, 20);
print(result); // Output: 30
```

## API Reference

### `PomeValue`

Represents any value in the Pome runtime.

-   `isNumber()`, `asNumber()`: Check/get double.
-   `isString()`, `asString()`: Check/get std::string.
-   `isBool()`, `asBool()`: Check/get bool.
-   `isNil()`: Check if nil.
-   `isList()`, `asList()`: Get `PomeList*`.
-   `isTable()`, `asTable()`: Get `PomeTable*`.

### `GarbageCollector` (`vm->getGC()`)

You must use the GC to allocate any Pome objects (Strings, Lists, Tables, Functions) to ensure they are managed correctly.

-   `allocate<T>(args...)`: Allocates an object of type T.

### `NativeFunction`

Wrapper for C++ functions.

-   Signature: `PomeValue func(const std::vector<PomeValue>& args)`

## Advanced: Rooting Objects

If you store `PomeValue`s or `PomeObject*`s in global C++ variables or structures that persist across function calls, you must protect them from the Garbage Collector. Currently, the API for registering external roots is not fully exposed, so it is recommended to keep Pome objects only within the scope of your native function calls or immediately return them to the interpreter.
