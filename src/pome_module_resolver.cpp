#include "../include/pome_module_resolver.h"
#include "../include/pome_file_utils.hpp"
#include "../include/pome_pkg_info.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>

namespace Pome {

    ModuleResolver::ModuleResolver() {
        // 1. Current directory and its 'modules' subdirectory
        addSearchPath(FileUtils::getCurrentPath());
        addSearchPath(FileUtils::getCurrentPath() + "/modules");
        addSearchPath(FileUtils::getCurrentPath() + "/examples/modules");
        addSearchPath(FileUtils::getCurrentPath() + "/test/root_tests");

        // 2. Virtual Environment: Walk up the tree for .pome_env
        std::string currentPath = FileUtils::getCurrentPath();
        std::string tempPath = currentPath;
        while (true) {
            if (FileUtils::exists(tempPath + "/.pome_env/lib")) {
                addSearchPath(tempPath + "/.pome_env/lib");
                break;
            }
            size_t lastSlash = tempPath.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == 0) {
                break;
            }
            tempPath = tempPath.substr(0, lastSlash);
        }

        // 3. POME_PATH environment variable
        const char *envPath = std::getenv("POME_PATH");
        if (envPath) {
            std::string pathList = envPath;
            size_t start = 0;
            size_t end = pathList.find(':'); // Unix style
            while (end != std::string::npos) {
                std::string p = pathList.substr(start, end - start);
                if (!p.empty()) {
                    addSearchPath(p);
                }
                start = end + 1;
                end = pathList.find(':', start);
            }
            std::string lastP = pathList.substr(start);
            if (!lastP.empty()) {
                addSearchPath(lastP);
            }
        }

        // 4. User home directory (~/.pome/modules)
        const char *homeDir = std::getenv("HOME");
        if (homeDir) {
            std::string userModules = std::string(homeDir) + "/.pome/modules";
            addSearchPath(userModules);
        }

        // 5. System directory
#ifdef _WIN32
        // Windows logic for system modules paths if needed.
#else
        addSearchPath("/usr/local/lib/pome/modules");
        addSearchPath("/usr/lib/pome/modules");
#endif
    }

    void ModuleResolver::addSearchPath(const std::string& path) {
        // Ensure trailing slash for all paths
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath.back() != '/') {
            cleanPath += "/";
        }
        searchPaths_.push_back(cleanPath);
    }

    std::string ModuleResolver::getNativeExtensionSuffix() {
#ifdef _WIN32
        return ".dll";
#elif __APPLE__
        return ".dylib";
#else
        return ".so";
#endif
    }

    bool ModuleResolver::isNativeModule(const std::string& pkgRoot, const std::string& moduleName) {
        std::string pkgJsonPath = pkgRoot + "/pome_pkg.json";
        if (FileUtils::exists(pkgJsonPath)) {
            try {
                // Using the inline reader from pome_pkg_info.h
                // Note: readPomePkgJson uses nlohmann/json.hpp
                // Make sure we have the implementation available.
                // The implementation is inline in header, so it's fine.
                // However, readPomePkgJson takes the directory path, not file path.
                // Wait, readPomePkgJson implementation: `std::string filePath = packagePath + "/pome_pkg.json";`
                // So pass `pkgRoot`.
                
                // We need to parse pome_pkg.json
                // The readPomePkgJson function is inline in header.
                // We need to check if nativeModules contains moduleName
                
                // Since readPomePkgJson is in namespace Pome, we can use it directly if header is included.
                // But wait, `readPomePkgJson` is defined in `include/pome_pkg_info.h` as `inline`.
                // It uses `nlohmann/json.hpp`.
                
                // Let's reimplement lightweight check to avoid heavy JSON dependency if possible?
                // No, stick to existing logic for correctness.
                
                // The header `pome_pkg_info.h` defines `readPomePkgJson`.
                // But wait, `pome_pkg_info.h` is included.
                
                // However, `readPomePkgJson` returns `PomePkgInfo` struct.
                // We need to check if moduleName is in `nativeModules` vector.
                
                // The `readPomePkgJson` function is defined in `include/pome_pkg_info.h`.
                // It is NOT a member of any class.
                
                // Let's call it.
                // Wait, `readPomePkgJson` throws if file not found or parse error.
                // We should wrap in try-catch.
                
                // But wait, `readPomePkgJson` is not visible?
                // Ah, it is in `include/pome_pkg_info.h`.
                
                // Let's check `pome_pkg_info.h` again.
                // It is `inline PomePkgInfo readPomePkgJson(const std::string& packagePath)`.
                
                // So we can use it.
                
                PomePkgInfo info = readPomePkgJson(pkgRoot);
                for (const auto& mod : info.nativeModules) {
                    if (mod == moduleName) return true;
                }
            } catch (...) {
                // Ignore errors
            }
        }
        return false;
    }

    ResolutionResult ModuleResolver::resolve(const std::string& logicalPath) {
        std::string pathSegment = logicalPath;
        size_t pos = 0;
        while ((pos = pathSegment.find('.', pos)) != std::string::npos) {
            pathSegment.replace(pos, 1, "/");
            pos++;
        }

        std::string moduleName = logicalPath;
        size_t lastDot = logicalPath.find_last_of('.');
        if (lastDot != std::string::npos) {
            moduleName = logicalPath.substr(lastDot + 1);
        }

        for (const auto& basePath : searchPaths_) {
            // Option 1: Try as a single file Pome module (e.g., basePath/my_pkg.pome)
            std::string pomeFileCandidate = basePath + pathSegment + ".pome";
            if (FileUtils::exists(pomeFileCandidate)) {
                return {ModuleType::POME_SCRIPT_FILE, pomeFileCandidate, moduleName};
            }

            // Option 2: Try as a package directory with __init__.pome (e.g., basePath/my_pkg/__init__.pome)
            std::string initFileCandidate = basePath + pathSegment + "/__init__.pome";
            if (FileUtils::exists(initFileCandidate)) {
                return {ModuleType::POME_PACKAGE_DIR, basePath + pathSegment, moduleName};
            }

            // Option 3: Try as a native module within a directory
            // This requires reading pome_pkg.json from the parent directory to find native_modules list.
            std::string currentModuleDir = basePath + pathSegment;
            // Determine the potential package root for pome_pkg.json
            std::string potentialPkgRoot;
            if (lastDot != std::string::npos) { // It's a submodule like my_pkg.sub_module
                std::string parentPathSegment = logicalPath.substr(0, lastDot); // "my_pkg"
                size_t parentPos = 0;
                while ((parentPos = parentPathSegment.find('.', parentPos)) != std::string::npos) {
                    parentPathSegment.replace(parentPos, 1, "/");
                    parentPos++;
                }
                potentialPkgRoot = basePath + parentPathSegment;
            } else { // It's a top-level module like "my_pkg"
                potentialPkgRoot = basePath + pathSegment;
            }

            // Check if native module is declared in package.json
            if (isNativeModule(potentialPkgRoot, moduleName)) {
                 std::string nativeLibPath = potentialPkgRoot + "/lib/" + moduleName + getNativeExtensionSuffix();
                 if (FileUtils::exists(nativeLibPath)) {
                     return {ModuleType::NATIVE_MODULE_FILE, nativeLibPath, moduleName};
                 }
            }
        }

        return {ModuleType::NOT_FOUND, "", ""};
    }

} // namespace Pome