#include "../include/pome_importer.h"
#include "../include/pome_value.h"
#include "../include/pome_interpreter.h"
#include "../include/pome_lexer.h"
#include "../include/pome_parser.h"
#include "../include/pome_stdlib.h" // Added standard library support
#include "../include/pome_environment.h"  // For Environment
#include "../include/pome_file_utils.hpp" // For FileUtils
#include "../include/pome_pkg_info.h"     // For PomePkgInfo

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm> // For std::find
#include <dlfcn.h>   // For dlopen, dlsym, dlclose (Unix-like dynamic loading)

// --- RAII Helper for Cyclic Import Detection ---
// This ensures that a module is removed from loadingModules_ even if an exception occurs
class ScopedLoadingModule
{
public:
    ScopedLoadingModule(std::set<std::string> &loadingModules, const std::string &moduleName)
        : loadingModules_(loadingModules), moduleName_(moduleName)
    {
        if (loadingModules_.count(moduleName_))
        {
            throw std::runtime_error("Cyclic import detected for module: " + moduleName_);
        }
        loadingModules_.insert(moduleName_);
    }
    ~ScopedLoadingModule()
    {
        loadingModules_.erase(moduleName_);
    }

private:
    std::set<std::string> &loadingModules_;
    const std::string moduleName_;
};

namespace Pome
{

    Importer::Importer(Interpreter &interpreter) : interpreter_(interpreter)
    {
        // 1. Current directory and its 'modules' subdirectory
        addSearchPath(FileUtils::getCurrentPath());
        addSearchPath(FileUtils::getCurrentPath() + "/modules");

        // 2. Virtual Environment: Walk up the tree for .pome_env
        std::string currentPath = FileUtils::getCurrentPath();
        std::string tempPath = currentPath;
        while (true)
        {
            if (FileUtils::exists(tempPath + "/.pome_env/lib"))
            {
                addSearchPath(tempPath + "/.pome_env/lib");
                break;
            }
            size_t lastSlash = tempPath.find_last_of('/');
            if (lastSlash == std::string::npos || lastSlash == 0)
            {
                break;
            }
            tempPath = tempPath.substr(0, lastSlash);
        }

        // 3. POME_PATH environment variable
        const char *envPath = std::getenv("POME_PATH");
        if (envPath)
        {
            std::string pathList = envPath;
            size_t start = 0;
            size_t end = pathList.find(':'); // Unix style
            while (end != std::string::npos)
            {
                std::string p = pathList.substr(start, end - start);
                if (!p.empty())
                {
                    addSearchPath(p);
                }
                start = end + 1;
                end = pathList.find(':', start);
            }
            std::string lastP = pathList.substr(start);
            if (!lastP.empty())
            {
                addSearchPath(lastP);
            }
        }

        // 4. User home directory (~/.pome/modules)
        const char *homeDir = std::getenv("HOME");
        if (homeDir)
        {
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

    PomeValue Importer::import(const std::string &logicalPath)
    {

        if (isCached(logicalPath))
        {
            return moduleCache_[logicalPath];
        }

        /**
         * Check for built-in modules
         */
        if (logicalPath == "math")
        {
            PomeModule* module = StdLib::createMathModule(interpreter_.getGC());
            PomeValue val(module);
            moduleCache_[logicalPath] = val;
            return val;
        }
        if (logicalPath == "io")
        {
            PomeModule* module = StdLib::createIOModule(interpreter_.getGC());
            PomeValue val(module);
            moduleCache_[logicalPath] = val;
            return val;
        }
        if (logicalPath == "string")
        {
            PomeModule* module = StdLib::createStringModule(interpreter_.getGC());
            PomeValue val(module);
            moduleCache_[logicalPath] = val;
            return val;
        }
        if (logicalPath == "time")
        {
            PomeModule* module = StdLib::createTimeModule(interpreter_.getGC());
            PomeValue val(module);
            moduleCache_[logicalPath] = val;
            return val;
        }

        ScopedLoadingModule loader(loadingModules_, logicalPath);
        ModuleCandidate candidate = findModuleCandidate(logicalPath);

        if (candidate.type == ModuleCandidateType::NOT_FOUND)
        {
            std::string searchInfo = "Searched in:\n";

            for (const auto &path : searchPaths_)
            {
                searchInfo += " - " + path + "\n";
            }

            throw std::runtime_error("ModuleNotFoundError: Module '" + logicalPath + "' not found.\n" + searchInfo);
        }

        PomeModule *moduleObj = interpreter_.getGC().allocate<PomeModule>();
        RootGuard moduleObjGuard(interpreter_.getGC(), moduleObj);
        PomeValue moduleValue(moduleObj);

        if (candidate.type == ModuleCandidateType::POME_SCRIPT_FILE || candidate.type == ModuleCandidateType::POME_PACKAGE_DIR)
        {
            std::string filePath = candidate.path;
            if (candidate.type == ModuleCandidateType::POME_PACKAGE_DIR)
            {
                filePath += "/__init__.pome";
            }

            std::string source = readFile(filePath);
            Lexer lexer(source);
            Parser parser(lexer);
            std::unique_ptr<Program> program = parser.parseProgram();

            if (!program)
            { // Defensive check
                throw std::runtime_error("Parsing failed for module: " + filePath);
            }
            moduleObj->ast_root = std::move(program); // Transfer ownership of AST to PomeModule

            // Execute the program in a new environment for the module
            Environment *previousEnvironment = interpreter_.currentEnvironment_;
            RootGuard prevEnvGuard(interpreter_.getGC(), previousEnvironment);
            Environment *moduleEnv = interpreter_.getGC().allocate<Environment>(&interpreter_, interpreter_.globalEnvironment_);
            interpreter_.currentEnvironment_ = moduleEnv;
            interpreter_.exportStack_.push_back(moduleObj);

            try
            {

                moduleObj->ast_root->accept(interpreter_); // Execute the module's program
            }
            catch (const std::exception &e)
            {

                // Restore state before re-throwing

                interpreter_.currentEnvironment_ = previousEnvironment;

                interpreter_.exportStack_.pop_back();

                throw; // Re-throw
            }

            interpreter_.currentEnvironment_ = previousEnvironment; // Restore previous environment

            interpreter_.exportStack_.pop_back(); // Pop module from export stack
        }
        else if (candidate.type == ModuleCandidateType::NATIVE_MODULE_FILE)
        {

            PomeValue nativeResult = interpreter_.loadNativeModule(candidate.path);

            if (nativeResult.isTable())
            {
                PomeTable *exportsTable = nativeResult.asTable();
                for (const auto &entry : exportsTable->elements)
                {
                    moduleObj->exports[entry.first] = entry.second;
                }
            }
            else if (!nativeResult.isNil())
            { // Native module returned something unexpected

                throw std::runtime_error("Native module '" + logicalPath + "' returned a non-Table, non-Nil value from PomeInitModule.");
            }
        }
        else
        {

            throw std::runtime_error("Internal Importer Error: Unhandled module candidate type for '" + logicalPath + "'.");
        }

        // Cache the PomeModule object wrapped in PomeValue

        std::unique_ptr<RootGuard> moduleGuard;

        if (moduleValue.isObject())
        {

            moduleGuard = std::make_unique<RootGuard>(interpreter_.getGC(), moduleValue.asObject());
        }

        moduleCache_[logicalPath] = moduleValue;

        return moduleValue;
    }

    void Importer::addSearchPath(const std::string &path)
    {
        // Ensure trailing slash for all paths
        std::string cleanPath = path;
        if (!cleanPath.empty() && cleanPath.back() != '/')
        {
            cleanPath += "/";
        }
        searchPaths_.push_back(cleanPath);
    }

    Importer::ModuleCandidate Importer::findModuleCandidate(const std::string &logicalPath)
    {
        std::string pathSegment = logicalPath;
        size_t pos = 0;
        while ((pos = pathSegment.find('.', pos)) != std::string::npos)
        {
            pathSegment.replace(pos, 1, "/");
            pos++;
        }

        std::string moduleName = logicalPath;
        size_t lastDot = logicalPath.find_last_of('.');
        if (lastDot != std::string::npos)
        {
            moduleName = logicalPath.substr(lastDot + 1);
        }

        for (const auto &basePath : searchPaths_)
        {
            // Option 1: Try as a single file Pome module (e.g., basePath/my_pkg.pome)
            std::string pomeFileCandidate = basePath + pathSegment + ".pome";
            if (FileUtils::exists(pomeFileCandidate))
            {
                return {ModuleCandidateType::POME_SCRIPT_FILE, pomeFileCandidate, moduleName};
            }

            // Option 2: Try as a package directory with __init__.pome (e.g., basePath/my_pkg/__init__.pome)
            std::string initFileCandidate = basePath + pathSegment + "/__init__.pome";
            if (FileUtils::exists(initFileCandidate))
            {
                return {ModuleCandidateType::POME_PACKAGE_DIR, basePath + pathSegment, moduleName};
            }

            // Option 3: Try as a native module within a directory
            // This requires reading pome_pkg.json from the parent directory to find native_modules list.
            std::string currentModuleDir = basePath + pathSegment;
            // Determine the potential package root for pome_pkg.json
            std::string potentialPkgRoot;
            if (lastDot != std::string::npos)
            {                                                                   // It's a submodule like my_pkg.sub_module
                std::string parentPathSegment = logicalPath.substr(0, lastDot); // "my_pkg"
                size_t parentPos = 0;
                while ((parentPos = parentPathSegment.find('.', parentPos)) != std::string::npos)
                {
                    parentPathSegment.replace(parentPos, 1, "/");
                    parentPos++;
                }
                potentialPkgRoot = basePath + parentPathSegment;
            }
            else
            { // It's a top-level module like "my_pkg"
                potentialPkgRoot = basePath + pathSegment;
            }

            std::string pkgJsonPath = potentialPkgRoot + "/pome_pkg.json";

            if (FileUtils::exists(pkgJsonPath))
            {
                try
                {
                    PomePkgInfo pkgInfo = readPomePkgJson(potentialPkgRoot);
                    // Check if current moduleName is listed as a native module in this package
                    auto it = std::find(pkgInfo.nativeModules.begin(), pkgInfo.nativeModules.end(), moduleName);
                    if (it != pkgInfo.nativeModules.end())
                    {
                        std::string nativeLibPath = potentialPkgRoot + "/lib/" + moduleName + getNativeExtensionSuffix();
                        if (FileUtils::exists(nativeLibPath))
                        {
                            return {ModuleCandidateType::NATIVE_MODULE_FILE, nativeLibPath, moduleName};
                        }
                    }
                }
                catch (const std::exception &e)
                {
                    // Ignore packages with malformed pome_pkg.json
                }
            }
        }

        return {ModuleCandidateType::NOT_FOUND, "", ""};
    }

    bool Importer::isCached(const std::string &logicalPath) const
    {
        return moduleCache_.count(logicalPath);
    }

    // Function to get platform-specific shared library extension
    std::string Importer::getNativeExtensionSuffix()
    {
#ifdef _WIN32
        return ".dll";
#elif __APPLE__
        return ".dylib";
#else
        return ".so";
#endif
    }

    std::string Importer::readFile(const std::string &path)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            throw std::runtime_error("Could not open file: " + path);
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }

} // namespace Pome
