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
    searchPaths_.push_back("./");
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
