cmake_minimum_required(VERSION 3.10)
project(pome LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add source files
set(SOURCE_FILES
    src/main.cpp
    src/pome_lexer.cpp
    src/pome_parser.cpp
    src/pome_value.cpp
    src/pome_environment.cpp
    src/pome_interpreter.cpp
    src/pome_stdlib.cpp
    src/pome_gc.cpp
    src/pome_importer.cpp
)

if(WIN32)
    # Add resource file for Windows icon
    # Ensure the resource compiler can find the icon file relative to the source
    list(APPEND SOURCE_FILES assets/pome.rc)
endif()

# Add the executable FIRST
add_executable(pome ${SOURCE_FILES})

# Add include directories
target_include_directories(pome PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# For Linux/Unix, copy the icon and desktop file to the build directory
if(NOT WIN32)
    configure_file(assets/pome.desktop ${CMAKE_BINARY_DIR}/pome.desktop COPYONLY)
    configure_file(assets/icon.png ${CMAKE_BINARY_DIR}/pome.png COPYONLY)
endif()
