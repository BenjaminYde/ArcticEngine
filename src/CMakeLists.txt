# configure target
set(ARCTIC_ENGINE_VERSION_MAJOR 0)
set(ARCTIC_ENGINE_VERSION_MINOR 1)
set(ARCTIC_ENGINE_VERSION_PATCH 0)

# create target
set(TARGET ArcticEngine)
add_executable(${TARGET} main.cpp)

# check platform
if(WIN32)
    # we are in windows
    target_compile_definitions(${TARGET} PUBLIC -DWIN32)

    # disable min max macro in windows headers
    # https://stackoverflow.com/questions/27442885/syntax-error-with-stdnumeric-limitsmax
    target_compile_definitions(${TARGET} PUBLIC -DNOMINMAX )


else()
    # throw error: no other OS supported
    message(FATAL_ERROR "Project must be compiled on Windows!")
endif()

#target_compile_options(${TARGET} PUBLIC /EHs) # enable exceptions

# link packages
FindPackage_Vulkan(${TARGET})
FindPackage_GLFW(  ${TARGET})
FindPackage_GLM(   ${TARGET})