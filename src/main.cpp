#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm> // Added for std::replace

#include "pome_lexer.h"
#include "pome_parser.h"
#include "pome_compiler.h"
#include "pome_vm.h"
#include "pome_chunk.h"
#include "pome_stdlib.h" // Added for stdlib
#include "../include/pome_module_resolver.h" // Added for ModuleResolver

// --- CONFIGURATION ---
const std::string POME_VERSION = "0.2.0-beta";

// ANSI Color Codes
const std::string RESET   = "\033[0m";
const std::string RED     = "\033[31m";
const std::string BOLD    = "\033[1m";
const std::string CYAN    = "\033[36m";
const std::string WHITE   = "\033[37m";

// --- SYSTEM INFO HELPERS ---
std::string getOS() {
    #ifdef _WIN32
    return "Windows";
    #elif __APPLE__
    return "macOS";
    #elif __linux__
    return "Linux";
    #else
    return "Unknown OS";
    #endif
}

std::string getCompiler() {
    #if defined(__clang__)
    return "Clang " + std::string(__clang_version__);
    #elif defined(__GNUC__)
    return "GCC " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
    #elif defined(_MSC_VER)
    return "MSVC " + std::to_string(_MSC_VER);
    #else
    return "Unknown Compiler";
    #endif
}

void printNeofetchStyle() {
    std::vector<std::string> logo = {
"                                                ",
"                         ======                 ",
"                        ==   ==                 ",
"                       ==  ==+                  ",
"                       =====                    ",
"                ======+ +======+                ",
"              ==+    =====    ====              ",
"             +=     +=   ==     ===             ",
"            ==      ==    ==     ==             ",
"            +=     === == ==+    ==             ",
"            ==     ==  ====      ==             ",
"             ==    ==  ==+=     ===             ",
"             ==+   ==  + ==     ==              ",
"              ==   ==          ==               ",
"               === ==         ==                ",
"                =====  ========                 ",
"                                                ",
    };

    std::vector<std::string> info;
    info.push_back(RED + BOLD + "USER" + RESET + "@" + RED + "PomeShell" + RESET);
    info.push_back("-------------");
    info.push_back(BOLD + "OS" + RESET + ":       " + getOS());
    info.push_back(BOLD + "Lang" + RESET + ":     Pome v" + POME_VERSION);
    info.push_back(BOLD + "Host" + RESET + ":     C++ STL / " + getCompiler());
    info.push_back(BOLD + "Mode" + RESET + ":     Interactive (REPL)");
    info.push_back(BOLD + "License" + RESET + ":  MIT");
    info.push_back(""); // Spacer
    info.push_back(CYAN + "Type 'exit' to quit." + RESET);

    size_t maxLines = std::max(logo.size(), info.size());
    int logoWidth = 30;

    std::cout << "\n"; // Top margin

    for (size_t i = 0; i < maxLines; ++i) {
        // Print Logo Segment (Red)
        if (i < logo.size()) {
            std::cout << RED << std::left << std::setw(logoWidth) << logo[i] << RESET;
        } else {
            std::cout << std::string(logoWidth, ' ');
        }

        // Print Info Segment (Text)
        if (i < info.size()) {
            std::cout << "  " << info[i];
        }

        std::cout << "\n";
    }
    std::cout << "\n"; // Bottom margin
}

// --- GLOBAL STATE ---
namespace Pome {
    class VM;
}
Pome::VM* currentVM = nullptr;

// --- CORE EXECUTION LOGIC ---

bool executeSource(const std::string& source) {
    try {
        Pome::Lexer lexer(source);
        Pome::Parser parser(lexer);
        std::unique_ptr<Pome::Program> program = parser.parseProgram();
        
        if (program) {
            Pome::GarbageCollector gc;
            Pome::ModuleResolver resolver;
            
            Pome::ModuleLoader loader = [&](const std::string& moduleName) -> Pome::PomeValue {
                // Built-in modules
                if (moduleName == "math") return Pome::PomeValue(Pome::StdLib::createMathModule(gc));
                if (moduleName == "io") return Pome::PomeValue(Pome::StdLib::createIOModule(gc));
                if (moduleName == "string") return Pome::PomeValue(Pome::StdLib::createStringModule(gc));
                if (moduleName == "time") return Pome::PomeValue(Pome::StdLib::createTimeModule(gc));

                Pome::ResolutionResult result = resolver.resolve(moduleName);

                if (result.type == Pome::ModuleType::NOT_FOUND) {
                    return Pome::PomeValue(); // Return nil if not found
                }

                if (result.type == Pome::ModuleType::POME_SCRIPT_FILE || result.type == Pome::ModuleType::POME_PACKAGE_DIR) {
                     std::string filePath = result.path;
                     if (result.type == Pome::ModuleType::POME_PACKAGE_DIR) {
                         filePath += "/__init__.pome";
                     }

                    std::ifstream mFile(filePath);
                    if (!mFile.is_open()) return Pome::PomeValue();
                    
                    std::stringstream mBuffer;
                    mBuffer << mFile.rdbuf();
                    mFile.close();
                    
                    // 2. Parse
                    Pome::Lexer mLexer(mBuffer.str());
                    Pome::Parser mParser(mLexer);
                    auto mProgram = mParser.parseProgram();
                    if (!mProgram) return Pome::PomeValue();
                    
                    // 3. Compile
                    Pome::Compiler mCompiler(gc);
                    auto mChunk = mCompiler.compile(*mProgram);
                    
                    // 4. Execute in a new module object
                    Pome::PomeModule* moduleObj = gc.allocate<Pome::PomeModule>();
                    
                    extern Pome::VM* currentVM; 
                    if (currentVM) {
                        currentVM->interpret(mChunk.get(), moduleObj);
                    }
                    
                    return Pome::PomeValue(moduleObj);
                }
                
                if (result.type == Pome::ModuleType::NATIVE_MODULE_FILE) {
                    Pome::PomeModule* moduleObj = gc.allocate<Pome::PomeModule>();
                    extern Pome::VM* currentVM;
                    if (currentVM) {
                         return currentVM->loadNativeModule(result.path, moduleObj);
                    }
                }
                
                return Pome::PomeValue();
            };

            Pome::Compiler compiler(gc);
            auto chunk = compiler.compile(*program);
            
            Pome::VM vm(gc, loader);
            
            // Set global VM pointer for the loader
            extern Pome::VM* currentVM;
            currentVM = &vm;

            // Register Standard Functions
            vm.registerGlobal("PI", Pome::PomeValue(3.141592653589793));
            
            vm.registerNative("print", [](const std::vector<Pome::PomeValue>& args) {
                for (size_t i = 0; i < args.size(); ++i) {
                    std::cout << args[i].toString();
                    if (i < args.size() - 1) std::cout << " ";
                }
                std::cout << std::endl;
                return Pome::PomeValue(std::monostate{});
            });

            vm.registerNative("len", [](const std::vector<Pome::PomeValue>& args) {
                if (args.empty()) return Pome::PomeValue(0.0);
                if (args[0].isString()) return Pome::PomeValue((double)args[0].asString().length());
                if (args[0].isList()) return Pome::PomeValue((double)args[0].asList()->elements.size());
                if (args[0].isTable()) return Pome::PomeValue((double)args[0].asTable()->elements.size());
                return Pome::PomeValue(0.0);
            });

            vm.registerNative("push", [&gc](const std::vector<Pome::PomeValue>& args) {
                if (args.size() < 2 || !args[0].isList()) return Pome::PomeValue(std::monostate{});
                args[0].asList()->elements.push_back(args[1]);
                gc.writeBarrier(args[0].asObject(), const_cast<Pome::PomeValue&>(args[1]));
                return Pome::PomeValue(std::monostate{});
            });

            vm.registerNative("tonumber", [](const std::vector<Pome::PomeValue>& args) {
                if (args.empty() || !args[0].isString()) return Pome::PomeValue(std::monostate{});
                try {
                    return Pome::PomeValue(std::stod(args[0].asString()));
                } catch (...) {
                    return Pome::PomeValue(std::monostate{});
                }
            });

            vm.registerNative("type", [&gc](const std::vector<Pome::PomeValue>& args) {
                if (args.empty()) return Pome::PomeValue(std::monostate{});
                if (args[0].isNil()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("nil"));
                if (args[0].isBool()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("boolean"));
                if (args[0].isNumber()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("number"));
                if (args[0].isString()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("string"));
                if (args[0].isList()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("list"));
                if (args[0].isTable()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("table"));
                if (args[0].isClass()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("class"));
                if (args[0].isInstance()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("instance"));
                if (args[0].isFunction()) return Pome::PomeValue(gc.allocate<Pome::PomeString>("function"));
                return Pome::PomeValue(gc.allocate<Pome::PomeString>("unknown"));
            });

            vm.registerNative("gc_count", [&gc](const std::vector<Pome::PomeValue>& args) {
                return Pome::PomeValue((double)gc.getObjectCount());
            });

            vm.registerNative("gc_collect", [&gc](const std::vector<Pome::PomeValue>& args) {
                gc.collect();
                return Pome::PomeValue(std::monostate{});
            });

            // Standard Library Modules are now loaded via ModuleLoader

            vm.interpret(chunk.get());
            if (vm.hasError) return false;
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << RED << "Error: " << RESET << e.what() << std::endl;
        return false;
    }
}

void runPrompt() {
    printNeofetchStyle();

    std::string line;
    while (true) {
        std::cout << RED << "pome" << RESET << "> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\nGoodbye!" << std::endl;
            break;
        }

        if (line == "exit") break;
        if (line.empty()) continue;

        executeSource(line);
    }
}

int runFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Could not open file '" << path << "'." << std::endl;
        return 74; 
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();

    if (!executeSource(source)) return 65;
    return 0;
}

void printUsage() {
    std::cout << "Usage: pome [script]" << std::endl;
    std::cout << "   Or: pome --version" << std::endl;
}

// --- MAIN ---
int main(int argc, char* argv[]) {
    if (argc == 1) {
        runPrompt();
    } 
    else if (argc == 2) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            printUsage();
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "Pome " << POME_VERSION << std::endl;
        } else {
            return runFile(arg);
        }
    } 
    else {
        std::cerr << "Too many arguments." << std::endl;
        return 64; 
    }
    return 0;
}
