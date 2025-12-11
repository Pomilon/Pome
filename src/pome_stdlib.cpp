#include "../include/pome_stdlib.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <fstream>
#include <sstream>

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

            registerNative(gc, module, "sin", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::sin(args[0].asNumber())); });

            registerNative(gc, module, "cos", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::cos(args[0].asNumber())); });

            registerNative(gc, module, "sqrt", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::sqrt(args[0].asNumber())); });

            registerNative(gc, module, "abs", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::abs(args[0].asNumber())); });

            registerNative(gc, module, "floor", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::floor(args[0].asNumber())); });

            registerNative(gc, module, "ceil", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::ceil(args[0].asNumber())); });

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



            /**
             * Capturing GC by reference:
             */
            registerNative(gc, module, "readFile", [&gc](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isString()) return PomeValue(std::monostate{});
        std::string path = args[0].asString();
        std::ifstream file(path);
        if (!file.is_open()) return PomeValue(std::monostate{}); 
        std::stringstream buffer;
        buffer << file.rdbuf();
        
        PomeString* s = gc.allocate<PomeString>(buffer.str());
        return PomeValue(s); });

            registerNative(gc, module, "writeFile", [](const std::vector<PomeValue> &args)
                           {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return PomeValue(false);
        std::string path = args[0].asString();
        std::string content = args[1].asString();
        std::ofstream file(path);
        if (!file.is_open()) return PomeValue(false);
        file << content;
        return PomeValue(true); });

            registerNative(gc, module, "input", [&gc](const std::vector<PomeValue> &args)
                           {
        if (!args.empty()) std::cout << args[0].toString(); 
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
        if (args.empty() || !args[0].isString()) return PomeValue(std::monostate{});
        std::string s = args[0].asString();
        
        if (args.size() < 2 || !args[1].isNumber()) {
             PomeString* newS = gc.allocate<PomeString>(s);
             return PomeValue(newS); 
        }
        
        size_t start = static_cast<size_t>(args[1].asNumber());
        if (start >= s.length()) {
             PomeString* empty = gc.allocate<PomeString>("");
             return PomeValue(empty);
        }
        
        size_t len = std::string::npos;
        if (args.size() >= 3 && args[2].isNumber()) {
            len = static_cast<size_t>(args[2].asNumber());
        }
        
        PomeString* sub = gc.allocate<PomeString>(s.substr(start, len));
        return PomeValue(sub); });

            return module;
        }

    }
}
