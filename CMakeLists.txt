# configure cmake version
cmake_minimum_required(VERSION 3.28 FATAL_ERROR)

# cmake include modules
include(FetchContent)
include(CMakePrintHelpers)

# configure cmake
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Arctic
        LANGUAGES "CXX")

#add_executable(Arctic main.cpp)
#return()

#cmake_print_variables(CMAKE_CXX_COMPILER_ID)
#cmake_print_variables(CMAKE_CXX_COMPILER_VERSION)
#cmake_print_variables(CMAKE_CXX_COMPILER)

# check platform
if(NOT UNIX)
    # throw error: no other OS supported
    message(FATAL_ERROR "Project must be compiled on Unix!")
endif()

# check compiler: force use clang (for the moment)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 19.1.5)
        message(FATAL_ERROR "Clang compiler version is smaller then 19.1.5! Current compiler version is ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
        message(FATAL_ERROR "Project must be compiled using preset! Current compiler is ${CMAKE_CXX_COMPILER_ID}")
endif()

# c++ flags
#add_compile_options(-Wshadow)

# defines
add_definitions(-DARCTIC_ASSETS_DIR="${CMAKE_CURRENT_LIST_DIR}/assets")

# set vulkan flags
add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
add_definitions(-DGLFW_EXPOSE_NATIVE_X11)
add_compile_options(-Wno-nullability-completeness) # for Vulkan Memory Allocator


# add sub directories
add_subdirectory(external)
add_subdirectory(src)