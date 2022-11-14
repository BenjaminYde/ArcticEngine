# configure target
set(ARCTIC_ENGINE_VERSION_MAJOR 0)
set(ARCTIC_ENGINE_VERSION_MINOR 1)
set(ARCTIC_ENGINE_VERSION_PATCH 0)

# create target
set(TARGET ArcticEngine)
message("target is ${TARGET}")
add_library(${TARGET} STATIC)

# set properties
set(INCLUDE_DIRS ${EDITOR_MODULE_DIR}/public)
set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${TARGET} PROPERTIES INCLUDE_DIRS ${INCLUDE_DIRS})

# set private sources
target_sources(${TARGET}
        PRIVATE
        arctic_engine.cpp
        vulkan_loader.cpp)

# set public sources
target_include_directories(${TARGET}
        PRIVATE
        ${INCLUDE_DIRS})

# check platform
if(WIN32)
    # we are in windows
    target_compile_definitions(${TARGET} PUBLIC -DWIN32)

    # disable min max macro in windows headers
    # https://stackoverflow.com/questions/27442885/syntax-error-with-stdnumeric-limitsmax
    target_compile_definitions(${TARGET} PUBLIC -DNOMINMAX)
else()
    # throw error: no other OS supported
    message(FATAL_ERROR "Project must be compiled on Windows!")
endif()

#target_compile_options(${TARGET} PUBLIC /EHs) # enable exceptions

# link packages
FindPackage_Vulkan(${TARGET})
FindPackage_GLFW(  ${TARGET})
FindPackage_GLM(   ${TARGET})