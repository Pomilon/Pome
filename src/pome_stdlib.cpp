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

        // Helper to register a native function into a map
        static void registerNative(std::shared_ptr<std::map<PomeValue, PomeValue>> &map,
                                   const std::string &name, NativeFn fn)
        {
            auto funcObj = std::make_shared<NativeFunction>(name, fn);
            (*map)[PomeValue(name)] = PomeValue(std::shared_ptr<PomeObject>(funcObj));
        }

        // --- Math Module ---

        std::shared_ptr<std::map<PomeValue, PomeValue>> createMathExports()
        {
            auto exports = std::make_shared<std::map<PomeValue, PomeValue>>();

            registerNative(exports, "sin", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::sin(args[0].asNumber())); });

            registerNative(exports, "cos", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::cos(args[0].asNumber())); });

            registerNative(exports, "sqrt", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::sqrt(args[0].asNumber())); });

            registerNative(exports, "abs", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::abs(args[0].asNumber())); });

            registerNative(exports, "floor", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::floor(args[0].asNumber())); });

            registerNative(exports, "ceil", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isNumber()) return PomeValue(std::monostate{});
        return PomeValue(std::ceil(args[0].asNumber())); });

            registerNative(exports, "random", [](const std::vector<PomeValue> &args)
                           {
        // Returns random between 0.0 and 1.0
        return PomeValue(static_cast<double>(std::rand()) / RAND_MAX); });

            // Constants
            (*exports)[PomeValue("pi")] = PomeValue(3.141592653589793);

            return exports;
        }

        // --- IO Module ---

        std::shared_ptr<std::map<PomeValue, PomeValue>> createIOExports()
        {
            auto exports = std::make_shared<std::map<PomeValue, PomeValue>>();

            registerNative(exports, "readFile", [](const std::vector<PomeValue> &args)
                           {
        if (args.empty() || !args[0].isString()) return PomeValue(std::monostate{});
        std::string path = args[0].asString();
        std::ifstream file(path);
        if (!file.is_open()) return PomeValue(std::monostate{}); // Error or nil
        std::stringstream buffer;
        buffer << file.rdbuf();
        return PomeValue(buffer.str()); });

            registerNative(exports, "writeFile", [](const std::vector<PomeValue> &args)
                           {
        if (args.size() < 2 || !args[0].isString() || !args[1].isString()) return PomeValue(false);
        std::string path = args[0].asString();
        std::string content = args[1].asString();
        std::ofstream file(path);
        if (!file.is_open()) return PomeValue(false);
        file << content;
        return PomeValue(true); });

            registerNative(exports, "input", [](const std::vector<PomeValue> &args)
                           {
        if (!args.empty()) std::cout << args[0].toString(); // Prompt
        std::string line;
        if (std::getline(std::cin, line)) {
            return PomeValue(line);
        }
        return PomeValue(std::monostate{}); });

            return exports;
        }

        // --- String Module ---

        std::shared_ptr<std::map<PomeValue, PomeValue>> createStringExports()
        {
            auto exports = std::make_shared<std::map<PomeValue, PomeValue>>();

            registerNative(exports, "sub", [](const std::vector<PomeValue> &args)
                           {
        // sub(str, start, [length])
        if (args.empty() || !args[0].isString()) return PomeValue(std::monostate{});
        std::string s = args[0].asString();
        
        if (args.size() < 2 || !args[1].isNumber()) return PomeValue(s); // Default to whole string? Or nil.
        
        size_t start = static_cast<size_t>(args[1].asNumber());
        if (start >= s.length()) return PomeValue("");
        
        size_t len = std::string::npos;
        if (args.size() >= 3 && args[2].isNumber()) {
            len = static_cast<size_t>(args[2].asNumber());
        }
        
        return PomeValue(s.substr(start, len)); });

            return exports;
        }

    }
}
