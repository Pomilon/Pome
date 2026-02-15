#ifndef POME_MODULE_RESOLVER_H
#define POME_MODULE_RESOLVER_H

#include <string>
#include <vector>
#include <set>

namespace Pome {

enum class ModuleType {
    NOT_FOUND,
    POME_SCRIPT_FILE,   // .pome
    POME_PACKAGE_DIR,   // directory with __init__.pome
    NATIVE_MODULE_FILE  // .so / .dll / .dylib
};

struct ResolutionResult {
    ModuleType type;
    std::string path;       // Absolute or relative path to file/dir
    std::string moduleName; // Resolved module name
};

class ModuleResolver {
public:
    ModuleResolver();

    /**
     * Add a directory to the module search path.
     */
    void addSearchPath(const std::string& path);

    /**
     * Resolve a logical module path (e.g., "my_pkg.sub_mod") to a physical file.
     */
    ResolutionResult resolve(const std::string& logicalPath);

    /**
     * Get the platform-specific shared library extension.
     */
    static std::string getNativeExtensionSuffix();

    const std::vector<std::string>& getSearchPaths() const { return searchPaths_; }

private:
    std::vector<std::string> searchPaths_;

    // Helper to check for pome_pkg.json
    bool isNativeModule(const std::string& pkgRoot, const std::string& moduleName);
};

} // namespace Pome

#endif // POME_MODULE_RESOLVER_H