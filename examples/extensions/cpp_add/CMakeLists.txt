cmake_minimum_required(VERSION 3.10)
project(my_extension)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Point to Pome includes
include_directories(../../include)

# Create shared library
add_library(my_extension SHARED my_extension.cpp)

# Output directory should be somewhere pome can find it (e.g., ../build)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../build)

# Link against libpome
# Locate the library file directly
set(LIBPOME_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../build/libpome.so)
target_link_libraries(my_extension ${LIBPOME_PATH})
