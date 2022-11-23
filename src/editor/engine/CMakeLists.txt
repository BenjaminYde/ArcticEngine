# configure target
set(ARCTIC_ENGINE_VERSION_MAJOR 0)
set(ARCTIC_ENGINE_VERSION_MINOR 1)
set(ARCTIC_ENGINE_VERSION_PATCH 0)

# create target
set(TARGET ArcticEngine)
message("target is ${TARGET}")
add_library(${TARGET} STATIC)

# set variables
set(INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(INCLUDE_DIRS_INTERNAL ${INCLUDE_DIRS}/engine)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

# set properties
set_target_properties(${TARGET} PROPERTIES
        INCLUDE_DIRS ${INCLUDE_DIRS})

# set sources
target_sources(${TARGET}
        PUBLIC
        ${INCLUDE_DIRS_INTERNAL}/arctic_engine.h
        PRIVATE
        ${SRC_DIR}/arctic_engine.cpp
        ${SRC_DIR}/vulkan_loader.cpp)

# set includes
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

# add module: utilities
target_link_libraries(${TARGET} PRIVATE Utilities)
get_target_property(Utilties_INCLUDE_DIRS Utilities INCLUDE_DIRS)
target_include_directories(${TARGET} PRIVATE ${Utilties_INCLUDE_DIRS})
