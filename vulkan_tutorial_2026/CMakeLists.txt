# configure cmake version
cmake_minimum_required(VERSION 4.3.3 FATAL_ERROR)

# signal to cmake to suppress developer warnings
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE INTERNAL "")

# generate compile commands (instead of vscode doing the copy)
if(CMAKE_EXPORT_COMPILE_COMMANDS)
  file(CREATE_LINK
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_SOURCE_DIR}/compile_commands.json
    SYMBOLIC
  )
endif()

# cmake include modules
include(FetchContent)
include(CMakePrintHelpers)

# external dependencies
add_subdirectory(cmake/external)
add_subdirectory(cmake/internal)

# set variables
set(TARGET ArcticEngine)

# set project
project(${TARGET}
        LANGUAGES "CXX")

# create target
add_executable(${TARGET})

# set defines
target_compile_definitions(${TARGET} PRIVATE PROJECT_ROOT_FOLDER="${CMAKE_SOURCE_DIR}")

# set tools
EnableClangTidy(TRUE)

# add packges
FindPackage_Vulkan(${TARGET})
FindPackage_SDL(${TARGET})
FindPackageTarget_stblog(${TARGET})

# set sources
target_sources(${TARGET}
        PRIVATE
        src/main.cpp)

# post build commands
compile_shaders_target(${TARGET})