#ifndef POME_FILE_UTILS_HPP
#define POME_FILE_UTILS_HPP

#include <string>
#include <vector>
#include <fstream> // For std::ifstream
#include <sys/stat.h> // For stat() on Unix-like systems
#include <unistd.h>   // For getcwd() on Unix-like systems
#ifdef _WIN32
#include <windows.h> // For GetCurrentDirectory on Windows
#include <direct.h>  // For _getcwd on Windows
#endif

namespace Pome {

class FileUtils {
public:
    // Get the current working directory
    static std::string getCurrentPath() {
        #ifdef _WIN32
            char buffer[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, buffer);
            return std::string(buffer);
        #else
            char buffer[1024];
            if (getcwd(buffer, sizeof(buffer)) != nullptr) {
                return std::string(buffer);
            }
            return ""; // Error
        #endif
    }

    // Check if a file or directory exists
    static bool exists(const std::string& path) {
        #ifdef _WIN32
            DWORD attributes = GetFileAttributesA(path.c_str());
            return (attributes != INVALID_FILE_ATTRIBUTES);
        #else
            struct stat buffer;
            return (stat(path.c_str(), &buffer) == 0);
        #endif
    }
};

} // namespace Pome

#endif // POME_FILE_UTILS_HPP