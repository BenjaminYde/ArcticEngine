# configure target
set(ARCTIC_ENGINE_VERSION_MAJOR 0)
set(ARCTIC_ENGINE_VERSION_MINOR 1)
set(ARCTIC_ENGINE_VERSION_PATCH 0)

# create target
set(TARGET ArcticEngine)
add_executable(${TARGET} main.cpp)

# link packages
FindPackage_Vulkan(${TARGET})
FindPackage_GLFW(  ${TARGET})
FindPackage_GLM(   ${TARGET})