# configure cmake version
cmake_minimum_required(VERSION 3.24 FATAL_ERROR)

# prevent in-source builds
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "In-source builds are not recommended. Please choose an empty directory outside the source folder.")
endif()

# cmake include modules
include(FetchContent)
include(CMakePrintHelpers)

# configure cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Arctic
        LANGUAGES "CXX")

#cmake_print_variables(CMAKE_CXX_COMPILER_ID)
#cmake_print_variables(CMAKE_CXX_COMPILER_VERSION)
#cmake_print_variables(CMAKE_CXX_COMPILER)

# check platform
if(NOT WIN32)
    # throw error: no other OS supported
    message(FATAL_ERROR "Project must be compiled on Windows!")
endif()

# check compiler: force use clang (for the moment)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 15.0.1)
        message(FATAL_ERROR "Clang compiler version is smaller then 15.0.1! Current compiler version is ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
else()
        message(FATAL_ERROR "Project must be compiled using preset! Current compiler is ${CMAKE_CXX_COMPILER_ID}")
endif()

# add sub directories
add_subdirectory(external)
add_subdirectory(src)