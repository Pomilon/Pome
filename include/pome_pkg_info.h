#ifndef POME_PKG_INFO_H
#define POME_PKG_INFO_H

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

// Assuming nlohmann/json.hpp is in external/nlohmann
#include "../external/nlohmann/json.hpp"
#include <fstream> // For std::ifstream

namespace Pome {

struct PomePkgInfo {
    std::string name;
    std::string version;
    std::string description;
    std::vector<std::string> authors;
    std::vector<std::string> nativeModules; // List of native module names within this package
    std::map<std::string, std::string> dependencies;

    // Default constructor
    PomePkgInfo() : name(""), version("0.0.0"), description("") {}
};

// Function to read and parse pome_pkg.json
inline PomePkgInfo readPomePkgJson(const std::string& packagePath) {
    std::string filePath = packagePath + "/pome_pkg.json";
    std::ifstream i(filePath);
    if (!i.is_open()) {
        throw std::runtime_error("Failed to open pome_pkg.json at: " + filePath);
    }

    nlohmann::json j;
    try {
        i >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("Failed to parse pome_pkg.json: " + std::string(e.what()));
    }

    PomePkgInfo info;
    info.name = j.value("name", "");
    info.version = j.value("version", "0.0.0");
    info.description = j.value("description", "");
    
    if (j.contains("authors") && j["authors"].is_array()) {
        for (const auto& author : j["authors"]) {
            info.authors.push_back(author.get<std::string>());
        }
    }

    if (j.contains("nativeModules") && j["nativeModules"].is_array()) {
        for (const auto& module : j["nativeModules"]) {
            info.nativeModules.push_back(module.get<std::string>());
        }
    }

    if (j.contains("dependencies") && j["dependencies"].is_object()) {
        for (auto const& [key, val] : j["dependencies"].items()) {
            info.dependencies[key] = val.get<std::string>();
        }
    }

    return info;
}

} // namespace Pome

#endif // POME_PKG_INFO_H