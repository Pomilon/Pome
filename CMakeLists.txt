cmake_minimum_required(VERSION 3.10)
project(pome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define library source files (everything except main.cpp)
set(LIB_SOURCES
    src/pome_lexer.cpp
    src/pome_parser.cpp
    src/pome_value.cpp
    src/pome_environment.cpp
    src/pome_interpreter.cpp
    src/pome_stdlib.cpp
    src/pome_gc.cpp
    src/pome_importer.cpp
    src/pome_file_utils.cpp
    src/pome_pkg_info.cpp # Added PomePkgInfo source
)

# Create the shared library
add_library(libpome SHARED ${LIB_SOURCES})
set_target_properties(libpome PROPERTIES OUTPUT_NAME "pome")

# Link against dynamic loading library (dl on Linux)
target_link_libraries(libpome PUBLIC ${CMAKE_DL_LIBS})

# Add include directories for the library
target_include_directories(libpome PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external # Added for nlohmann/json
)

# Define executable source files
set(EXE_SOURCES src/main.cpp)

if(WIN32)
    # Add resource file for Windows icon to the executable
    list(APPEND EXE_SOURCES assets/pome.rc)
endif()

# Create the executable
add_executable(pome ${EXE_SOURCES})

# Link the executable against the library
target_link_libraries(pome PRIVATE libpome)

# Add include directories for the executable (so it can find headers to interface with libpome)
target_include_directories(pome PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# For Linux/Unix, copy the icon and desktop file to the build directory
if(NOT WIN32)
    configure_file(assets/pome.desktop ${CMAKE_BINARY_DIR}/pome.desktop COPYONLY)
    configure_file(assets/icon.png ${CMAKE_BINARY_DIR}/pome.png COPYONLY)
endif()
