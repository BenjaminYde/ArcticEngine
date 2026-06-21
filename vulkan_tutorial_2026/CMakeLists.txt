# configure cmake version
cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

# signal to cmake to suppress developer warnings
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "")

# cmake include modules
include(FetchContent)
include(CMakePrintHelpers)

# external dependencies
add_subdirectory(external)

# set variables
set(TARGET_NAME ArcticEngine)

# set project
project(${TARGET_NAME}
        LANGUAGES "CXX")

# c++ flags
add_compile_options(-Wshadow)

# create target
add_executable(${TARGET_NAME})

# add packges
FindPackage_Vulkan(${TARGET_NAME})


# set sources
target_sources(${TARGET_NAME}
        PRIVATE
        src/main.cpp)