#include "../include/pome_stdlib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono> // Added for Time Module
#include <thread> // Added for Time Module
#include <optional> // Added for arg parsing helper

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

            return module;
        }

        /**
         * --- Time Module ---
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

    }
}
