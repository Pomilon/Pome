#include "../include/pome_importer.h"
#include "../include/pome_interpreter.h"
#include "../include/pome_lexer.h"
#include "../include/pome_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Pome {

Importer::Importer(Interpreter& interpreter) : interpreter_(interpreter) {
    // 1. Current directory
    searchPaths_.push_back("./");
    searchPaths_.push_back("./modules/");

    // 2. POME_PATH environment variable
    const char* envPath = std::getenv("POME_PATH");
    if (envPath) {
        std::string pathList = envPath;
        size_t start = 0;
        size_t end = pathList.find(':'); // Unix style
        while (end != std::string::npos) {
            std::string p = pathList.substr(start, end - start);
            if (!p.empty()) {
                if (p.back() != '/') p += "/";
                searchPaths_.push_back(p);
            }
            start = end + 1;
            end = pathList.find(':', start);
        }
        std::string lastP = pathList.substr(start);
        if (!lastP.empty()) {
            if (lastP.back() != '/') lastP += "/";
            searchPaths_.push_back(lastP);
        }
    }

    // 3. User home directory (~/.pome/modules)
    const char* homeDir = std::getenv("HOME");
    if (homeDir) {
        std::string userModules = std::string(homeDir) + "/.pome/modules/";
        searchPaths_.push_back(userModules);
    }

    // 4. System directory
#ifdef _WIN32
    // Windows logic if needed, usually relative to executable
#else
    searchPaths_.push_back("/usr/local/lib/pome/modules/");
    searchPaths_.push_back("/usr/lib/pome/modules/");
#endif
}

std::shared_ptr<Program> Importer::import(const std::string& logicalPath) {
    if (isCached(logicalPath)) {
        return moduleCache_[logicalPath];
    }

    std::string filePath = resolvePath(logicalPath);
    if (filePath.empty()) {
        throw std::runtime_error("Module not found: " + logicalPath);
    }

    std::string source = readFile(filePath);
    
    /**
     * Parse the module
     */
    Lexer lexer(source);
    Parser parser(lexer);
    auto program = parser.parseProgram(); // Returns unique_ptr<Program>
    
    /**
     * Convert to shared_ptr for caching
     */
    std::shared_ptr<Program> sharedProgram = std::move(program);
    moduleCache_[logicalPath] = sharedProgram;

    return sharedProgram;
}

std::string Importer::resolvePath(const std::string& logicalPath) {
    /**
     * Convert logical path "a.b" -> "a/b.pome"
     */
    std::string path = logicalPath;
    size_t pos = 0;
    while ((pos = path.find('.', pos)) != std::string::npos) {
        path.replace(pos, 1, "/");
        pos++;
    }
    path += ".pome";

    for (const auto& searchPath : searchPaths_) {
        std::string fullPath = searchPath + path;
        /**
         * Check if file exists
         */
        std::ifstream f(fullPath);
        if (f.good()) {
            return fullPath;
        }
    }

    return "";
}

bool Importer::isCached(const std::string& logicalPath) const {
    return moduleCache_.find(logicalPath) != moduleCache_.end();
}

void Importer::addSearchPath(const std::string& path) {
    searchPaths_.push_back(path);
}

std::string Importer::readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace Pome
