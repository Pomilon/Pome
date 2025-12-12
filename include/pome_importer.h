#ifndef POME_IMPORTER_H
#define POME_IMPORTER_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <set> // For cyclic import detection

#include "pome_file_utils.hpp" // For path utilities
#include "pome_pkg_info.h"     // For PomePkgInfo

namespace Pome {

class Interpreter; // Forward declaration
class Program; // Forward declaration
class PomeValue; // Forward declaration for native ABI
class PomeModule; // Forward declaration for PomeModule*

class Importer {
public:
    explicit Importer(Interpreter& interpreter);

    PomeValue import(const std::string& logicalPath);

    // This method is now public as Interpreter needs to add paths to importer
    void addSearchPath(const std::string& path);

    // Public getter for moduleCache_ so Interpreter can mark it as GC root
    const std::map<std::string, PomeValue>& getModuleCache() const { return moduleCache_; }

private:
    Interpreter& interpreter_;
    std::vector<std::string> searchPaths_;
    std::map<std::string, PomeValue> moduleCache_; // Changed to store PomeValue
    std::set<std::string> loadingModules_; // For cyclic import detection

    // Helper for module candidate types
    enum class ModuleCandidateType {
        NOT_FOUND,
        POME_SCRIPT_FILE,
        POME_PACKAGE_DIR,
        NATIVE_MODULE_FILE
    };

    struct ModuleCandidate {
        ModuleCandidateType type = ModuleCandidateType::NOT_FOUND;
        std::string path; // Resolved absolute path (file or directory)
        std::string moduleName; // The base module name (e.g., 'sub_native' from 'my_pkg.sub_native')
    };

    ModuleCandidate findModuleCandidate(const std::string& logicalPath);
    std::string resolveBasePath(const std::string& logicalPath); // Old resolvePath modified
    bool isCached(const std::string& logicalPath) const;
        std::string readFile(const std::string& path);
        // PomeValue loadNativeModule(const std::string& libraryPath); // Moved to Interpreter
    

    // Helper to get platform-specific shared library extension
    std::string getNativeExtensionSuffix();
};

} // namespace Pome

#endif // POME_IMPORTER_H