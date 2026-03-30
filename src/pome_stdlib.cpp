#include "../include/pome_stdlib.h"
#include "../include/pome_vm.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono> // Added for Time Module
#include <thread> // Added for Time Module
#include <optional> // Added for arg parsing helper

#include <dlfcn.h>
#include <ffi.h>

namespace Pome
{
    namespace StdLib
    {

        /**
         * Helper to register a native function into a module
         */
        static void registerNative(GarbageCollector &gc, PomeModule *module,
                                   const std::string &name, NativeFn fn)
        {
            NativeFunction *funcObj = gc.allocate<NativeFunction>(name, fn);

            /**
             * Create key string
             */
            PomeString *keyStr = gc.allocate<PomeString>(name);

            module->exports[PomeValue(keyStr)] = PomeValue(funcObj);
        }

        /**
         * --- Math Module ---
         */

        PomeModule *createMathModule(GarbageCollector &gc)
        {
            PomeModule *module = gc.allocate<PomeModule>();

            auto getNumberArg = [](const std::vector<PomeValue>& args, size_t index) -> std::optional<double> {
                size_t realIndex = index;
                if (!args.empty() && args[0].isModule()) realIndex++;
                if (realIndex < args.size() && args[realIndex].isNumber()) return args[realIndex].asNumber();
                return std::nullopt;
            };

            registerNative(gc, module, "sin", [getNumberArg](const std::vector<PomeValue> &args)
                           {
        auto val = getNumberArg(args, 0);
        if (!val) return PomeValue(std::monostate{});
        return PomeValue(std::sin(*val)); });

            registerNative(gc, module, "cos", [getNumberArg](const std::vector<PomeValue> &args)
                           {
        auto val = getNumberArg(args, 0);
        if (!val) return PomeValue(std::monostate{});
        return PomeValue(std::cos(*val)); });

            registerNative(gc, module, "sqrt", [getNumberArg](const std::vector<PomeValue> &args)
                           {
        auto val = getNumberArg(args, 0);
        if (!val) return PomeValue(std::monostate{});
        return PomeValue(std::sqrt(*val)); });

            registerNative(gc, module, "abs", [getNumberArg](const std::vector<PomeValue> &args)
                           {
        auto val = getNumberArg(args, 0);
        if (!val) return PomeValue(std::monostate{});
        return PomeValue(std::abs(*val)); });

            registerNative(gc, module, "floor", [getNumberArg](const std::vector<PomeValue> &args)
                           {
        auto val = getNumberArg(args, 0);
        if (!val) return PomeValue(std::monostate{});
        return PomeValue(std::floor(*val)); });

            registerNative(gc, module, "ceil", [getNumberArg](const std::vector<PomeValue> &args)
                           {
        auto val = getNumberArg(args, 0);
        if (!val) return PomeValue(std::monostate{});
        return PomeValue(std::ceil(*val)); });

            registerNative(gc, module, "random", [](const std::vector<PomeValue> &args)
                           { return PomeValue(static_cast<double>(std::rand()) / RAND_MAX); });

            /**
             * Constants
             */
            PomeString *piStr = gc.allocate<PomeString>("pi");
            module->exports[PomeValue(piStr)] = PomeValue(3.141592653589793);

            return module;
        }

        /**
         * --- IO Module ---
         */

        PomeModule *createIOModule(GarbageCollector &gc)
        {
            PomeModule *module = gc.allocate<PomeModule>();

            auto getStringArg = [](const std::vector<PomeValue>& args, size_t index) -> std::optional<std::string> {
                size_t realIndex = index;
                if (!args.empty() && args[0].isModule()) realIndex++;
                if (realIndex < args.size() && args[realIndex].isString()) return args[realIndex].asString();
                return std::nullopt;
            };

            registerNative(gc, module, "readFile", [&gc, getStringArg](const std::vector<PomeValue> &args)
                           {
        auto path = getStringArg(args, 0);
        if (!path) return PomeValue(std::monostate{});
        std::ifstream file(*path);
        if (!file.is_open()) return PomeValue(std::monostate{}); 
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        PomeString* s = gc.allocate<PomeString>(buffer.str());
        return PomeValue(s); });

            registerNative(gc, module, "writeFile", [getStringArg](const std::vector<PomeValue> &args)
                           {
        auto path = getStringArg(args, 0);
        auto content = getStringArg(args, 1);
        if (!path || !content) return PomeValue(false);
        std::ofstream file(*path);
        if (!file.is_open()) return PomeValue(false);
        file << *content;
        return PomeValue(true); });

            registerNative(gc, module, "input", [&gc](const std::vector<PomeValue> &args)
                           {
        size_t idx = 0;
        if (!args.empty() && args[0].isModule()) idx++;
        if (args.size() > idx) std::cout << args[idx].toString(); 
        std::string line;
        if (std::getline(std::cin, line)) {
            PomeString* s = gc.allocate<PomeString>(line);
            return PomeValue(s);
        }
        return PomeValue(std::monostate{}); });

            return module;
        }

        /**
         * --- String Module ---
         */

        PomeModule *createStringModule(GarbageCollector &gc)
        {
            PomeModule *module = gc.allocate<PomeModule>();

            registerNative(gc, module, "sub", [&gc](const std::vector<PomeValue> &args)
                           {
        size_t idx = 0;
        if (!args.empty() && args[0].isModule()) idx++;

        if (args.size() <= idx || !args[idx].isString()) return PomeValue(std::monostate{});
        std::string s = args[idx].asString();
        
        if (args.size() < idx + 2 || !args[idx + 1].isNumber()) {
             PomeString* newS = gc.allocate<PomeString>(s);
             return PomeValue(newS); 
        }
        
        size_t start = static_cast<size_t>(args[idx + 1].asNumber());
        if (start >= s.length()) {
             PomeString* empty = gc.allocate<PomeString>("");
             return PomeValue(empty);
        }
        
        size_t len = std::string::npos;
        if (args.size() >= idx + 3 && args[idx + 2].isNumber()) {
            len = static_cast<size_t>(args[idx + 2].asNumber());
        }
        
        PomeString* sub = gc.allocate<PomeString>(s.substr(start, len));
        return PomeValue(sub); });

            registerNative(gc, module, "lower", [&gc](const std::vector<PomeValue> &args)
            {
                size_t idx = 0;
                if (!args.empty() && args[0].isModule()) idx++;
                if (args.size() <= idx || !args[idx].isString()) return PomeValue(std::monostate{});
                std::string s = args[idx].asString();
                for (auto &c : s) c = std::tolower(c);
                return PomeValue(gc.allocate<PomeString>(s));
            });

            registerNative(gc, module, "upper", [&gc](const std::vector<PomeValue> &args)
            {
                size_t idx = 0;
                if (!args.empty() && args[0].isModule()) idx++;
                if (args.size() <= idx || !args[idx].isString()) return PomeValue(std::monostate{});
                std::string s = args[idx].asString();
                for (auto &c : s) c = std::toupper(c);
                return PomeValue(gc.allocate<PomeString>(s));
            });

            return module;
        }

        // Add pop, shift to io or a new module? 
        // Actually, we need a 'table' or 'list' module.
        // For now, let's put them in a global scope in main.cpp or a 'list' module.
        // Let's add a List module.
        
        PomeModule *createListModule(GarbageCollector &gc)
        {
            PomeModule *module = gc.allocate<PomeModule>();

            registerNative(gc, module, "push", [&gc](const std::vector<PomeValue> &args)
            {
                size_t idx = 0;
                if (!args.empty() && args[0].isModule()) idx++;
                if (args.size() < idx + 2 || !args[idx].isList()) return PomeValue(std::monostate{});
                args[idx].asList()->elements.push_back(args[idx + 1]);
                gc.writeBarrier(args[idx].asObject(), const_cast<PomeValue&>(args[idx + 1]));
                return PomeValue(std::monostate{});
            });

            registerNative(gc, module, "pop", [](const std::vector<PomeValue> &args)
            {
                size_t idx = 0;
                if (!args.empty() && args[0].isModule()) idx++;
                if (args.size() <= idx || !args[idx].isList()) return PomeValue(std::monostate{});
                auto& elements = args[idx].asList()->elements;
                if (elements.empty()) return PomeValue(std::monostate{});
                PomeValue val = elements.back();
                elements.pop_back();
                return val;
            });

            return module;
        }
/**
 * --- Threading Module ---
 */
PomeModule *createThreadingModule(GarbageCollector &gc, ModuleLoader loader)
{
    PomeModule *module = gc.allocate<PomeModule>();

    registerNative(gc, module, "spawn", [&gc, loader](const std::vector<PomeValue> &args)
    {
        if (args.size() < 1 || !args[0].isPomeFunction()) {
            return PomeValue(); // Needs a function
        }

        PomeFunction* originalFn = args[0].asPomeFunction();

        PomeThread* threadObj = gc.allocate<PomeThread>();

        // Capture and deep-copy arguments for the thread
        std::vector<PomeValue> originalArgs;
        for (size_t i = 1; i < args.size(); ++i) originalArgs.push_back(args[i]);

        threadObj->handle = std::thread([originalFn, originalArgs, loader, threadObj]() {
            auto threadGC = std::make_unique<GarbageCollector>();
            VM threadVM(*threadGC, loader);

            std::map<PomeObject*, PomeObject*> copiedObjects;
            PomeFunction* clonedFn = (PomeFunction*)PomeValue(originalFn).deepCopy(*threadGC, copiedObjects).asObject();

            std::vector<PomeValue> threadArgs;
            for (auto& arg : originalArgs) {
                threadArgs.push_back(arg.deepCopy(*threadGC, copiedObjects));
            }

            // Register standard globals for the new isolate
            threadVM.registerGlobal("PI", PomeValue(3.141592653589793));
            threadVM.registerNative("print", [](const std::vector<PomeValue>& args) {
                for (size_t i = 0; i < args.size(); ++i) {
                    std::cout << args[i].toString() << (i == args.size() - 1 ? "" : " ");
                }
                std::cout << std::endl;
                return PomeValue(std::monostate{});
            });

            try {
                threadObj->result = threadVM.interpret(clonedFn->chunk.get(), clonedFn->module);
            } catch (const std::exception& e) {
                std::cerr << "[Thread] Fatal error in isolate: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[Thread] Unknown fatal error in isolate." << std::endl;
            }
            threadObj->isFinished = true;
            threadObj->isolateGC = std::move(threadGC); // Move it to threadObj to keep it alive
        });
        return PomeValue(threadObj);
    });

    registerNative(gc, module, "join", [&gc](const std::vector<PomeValue> &args) {
        if (args.empty() || !args[0].isObject() || args[0].asObject()->type() != ObjectType::THREAD) {
            return PomeValue(std::monostate{});
        }
        PomeThread* threadObj = (PomeThread*)args[0].asObject();
        if (threadObj->handle.joinable()) {
            threadObj->handle.join();
        }
        
        // At this point, the thread is finished, and isolateGC keeps its heap alive.
        // We MUST deep-copy the result into the current VM's GC heap.
        std::map<PomeObject*, PomeObject*> copiedObjects;
        PomeValue result = threadObj->result.deepCopy(gc, copiedObjects);
        
        // After deep-copying, we can release the child heap.
        threadObj->isolateGC.reset();
        
        return result;
    });

    return module;
}

/**
 * --- Time Module ---
...
         */

        PomeModule *createTimeModule(GarbageCollector &gc)
        {
            PomeModule *module = gc.allocate<PomeModule>();

            // clock(): Returns monotonic time in seconds (with sub-second precision)
            registerNative(gc, module, "clock", [](const std::vector<PomeValue> &args)
                           {
                auto now = std::chrono::high_resolution_clock::now();
                auto duration = now.time_since_epoch();
                double seconds = std::chrono::duration<double>(duration).count();
                return PomeValue(seconds); 
            });

            // sleep(seconds): Sleep for n seconds
            registerNative(gc, module, "sleep", [](const std::vector<PomeValue> &args)
                           {
                if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
                double seconds = args[0].asNumber();
                std::this_thread::sleep_for(std::chrono::duration<double>(seconds));
                return PomeValue(std::monostate{}); 
            });

            return module;
        }

        /**
         * --- FFI Module ---
         */
        PomeModule *createFFIModule(GarbageCollector &gc)
        {
            PomeModule *module = gc.allocate<PomeModule>();

            registerNative(gc, module, "load", [&gc](const std::vector<PomeValue> &args)
                           {
                if (args.empty() || !args[0].isString()) return PomeValue(std::monostate{});
                void* handle = dlopen(args[0].asString().c_str(), RTLD_LAZY);
                if (!handle) {
                    std::cerr << "FFI Error: " << dlerror() << std::endl;
                    return PomeValue(std::monostate{});
                }
                return PomeValue(gc.allocate<PomeNativeObject>(handle, "lib")); 
            });

            registerNative(gc, module, "get", [&gc](const std::vector<PomeValue> &args)
                           {
                if (args.size() < 2 || !args[0].isObject() || args[0].asObject()->type() != ObjectType::NATIVE_OBJECT || !args[1].isString()) {
                    return PomeValue(std::monostate{});
                }
                void* handle = ((PomeNativeObject*)args[0].asObject())->ptr;
                void* addr = dlsym(handle, args[1].asString().c_str());
                if (!addr) {
                    std::cerr << "FFI Error: " << dlerror() << std::endl;
                    return PomeValue(std::monostate{});
                }
                return PomeValue(gc.allocate<PomeNativeObject>(addr, "func")); 
            });

            registerNative(gc, module, "call", [&gc](const std::vector<PomeValue> &args)
                           {
                if (args.size() < 4 || !args[0].isObject() || args[0].asObject()->type() != ObjectType::NATIVE_OBJECT) {
                    return PomeValue(std::monostate{});
                }
                void* addr = ((PomeNativeObject*)args[0].asObject())->ptr;
                std::string retTypeStr = args[1].toString();
                
                ffi_cif cif;
                ffi_type *types[16];
                void *values[16];
                
                // Storage for different types
                double d_args[16];
                int i_args[16];
                const char* s_args[16];
                
                int argCount = 0;
                if (args[2].isList() && args[3].isList()) {
                    PomeList* typeList = args[2].asList();
                    PomeList* valList = args[3].asList();
                    argCount = (int)valList->elements.size();
                    for (int i = 0; i < argCount && i < 16; ++i) {
                        std::string t = typeList->elements[i].toString();
                        if (t == "int") {
                            types[i] = &ffi_type_sint;
                            i_args[i] = (int)valList->elements[i].asNumber();
                            values[i] = &i_args[i];
                        } else if (t == "string") {
                            types[i] = &ffi_type_pointer;
                            s_args[i] = valList->elements[i].asString().c_str();
                            values[i] = &s_args[i];
                        } else {
                            types[i] = &ffi_type_double;
                            d_args[i] = valList->elements[i].asNumber();
                            values[i] = &d_args[i];
                        }
                    }
                }

                ffi_type* retType = &ffi_type_void;
                if (retTypeStr == "int") retType = &ffi_type_sint;
                else if (retTypeStr == "double") retType = &ffi_type_double;
                else if (retTypeStr == "string") retType = &ffi_type_pointer;
                
                if (ffi_prep_cif(&cif, FFI_DEFAULT_ABI, argCount, retType, types) == FFI_OK) {
                    if (retType == &ffi_type_double) {
                        double result;
                        ffi_call(&cif, FFI_FN(addr), &result, values);
                        return PomeValue(result);
                    } else if (retType == &ffi_type_sint) {
                        int result;
                        ffi_call(&cif, FFI_FN(addr), &result, values);
                        return PomeValue((double)result);
                    } else if (retType == &ffi_type_pointer) {
                        char* result;
                        ffi_call(&cif, FFI_FN(addr), &result, values);
                        if (result) return PomeValue(gc.allocate<PomeString>(result));
                        return PomeValue(std::monostate{});
                    } else {
                        ffi_call(&cif, FFI_FN(addr), nullptr, values);
                        return PomeValue(std::monostate{});
                    }
                }
                
                return PomeValue(std::monostate{});
            });

            return module;
        }

    }
}
