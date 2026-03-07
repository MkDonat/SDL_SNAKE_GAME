cmake_minimum_required(VERSION 3.16)

project(
    SNAKE
    VERSION 0.0.1
    DESCRIPTION "A classic snake game."
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set the output directory for built objects.
# This makes sure that the dynamic library goes into the build directory automatically.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")

# This assumes the SDL source is available in vendored/SDL
add_subdirectory(SDL EXCLUDE_FROM_ALL)

# Adding engine sub dir
add_subdirectory(engine)

# Create your game executable target as usual
add_executable(exec WIN32 main.cpp)

# Link to the actual SDL3 library.
target_link_libraries(exec PRIVATE SDL3::SDL3)