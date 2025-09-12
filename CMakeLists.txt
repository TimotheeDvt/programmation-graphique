cmake_minimum_required(VERSION 3.10)
project(C1 LANGUAGES CXX)

# Use vcpkg
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Build executable
add_executable(main main.cpp)

# GLEW paths
set(GLEW_INCLUDE_DIR "C:/libs/glew/include")
set(GLEW_LIBRARY "C:/libs/glew/lib/Release/x64/glew32.lib")

# Include and link
target_include_directories(main PRIVATE ${GLEW_INCLUDE_DIR})
target_link_libraries(main PRIVATE glfw OpenGL::GL ${GLEW_LIBRARY})
