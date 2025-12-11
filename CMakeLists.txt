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

# Add the executable FIRST
add_executable(pome ${SOURCE_FILES})

# Add include directories
target_include_directories(pome PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
