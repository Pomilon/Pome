cmake_minimum_required(VERSION 3.10)
project(my_extension)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Create shared library
add_library(my_extension SHARED my_extension.cpp)

# Point to Pome includes
target_include_directories(my_extension PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../../)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../build)
