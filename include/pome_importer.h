#ifndef POME_IMPORTER_H
#define POME_IMPORTER_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include "pome_ast.h"
#include "pome_value.h"

namespace Pome {

class Interpreter;

class Importer {
public:
    explicit Importer(Interpreter& interpreter);

    std::shared_ptr<Program> import(const std::string& logicalPath);

    /**
     * Resolves "a.b" -> "root/a/b.pome"
     */
    std::string resolvePath(const std::string& logicalPath);
    
    /**
     * Check if module is already loaded
     */
    bool isCached(const std::string& logicalPath) const;
    
    /**
     * Add a search path
     */
    void addSearchPath(const std::string& path);

private:
    Interpreter& interpreter_;
    std::map<std::string, std::shared_ptr<Program>> moduleCache_;
    std::vector<std::string> searchPaths_;

    std::string readFile(const std::string& path);
};

} // namespace Pome

#endif // POME_IMPORTER_H
