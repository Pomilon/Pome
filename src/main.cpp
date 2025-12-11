#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "pome_lexer.h"
#include "pome_parser.h"
#include "pome_interpreter.h"

// --- CONFIGURATION ---
const std::string POME_VERSION = "0.1.0";

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

// --- CORE EXECUTION LOGIC ---

bool executeSource(const std::string& source, Pome::Interpreter& interpreter) {
    try {
        Pome::Lexer lexer(source);
        Pome::Parser parser(lexer);
        std::unique_ptr<Pome::Program> program = parser.parseProgram();
        
        if (program) {
            interpreter.interpret(*program);
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << RED << "Error: " << RESET << e.what() << std::endl;
        return false;
    }
}

void runPrompt() {
    printNeofetchStyle(); // Show the cool UI

    Pome::Interpreter interpreter; 
    std::string line;

    while (true) {
        std::cout << RED << "pome" << RESET << "> "; // Colored prompt
        
        if (!std::getline(std::cin, line)) {
            std::cout << "\nGoodbye!" << std::endl;
            break;
        }

        if (line == "exit") break;
        if (line.empty()) continue;

        executeSource(line, interpreter);
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

    Pome::Interpreter interpreter;
    if (!executeSource(source, interpreter)) return 65;
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