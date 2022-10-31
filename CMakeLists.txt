# configure cmake
cmake_minimum_required(VERSION 3.24)
include(FetchContent)

# configure project
set(CMAKE_CXX_STANDARD 20)
project(arctic_engine LANGUAGES "CXX")

#configure target
add_executable(arctic_engine main.cpp)
#target_compile_options (arctic_engine PRIVATE -fexceptions)

# function to find vulkan
function(FindVulkan TARGET_NAME VKLIB_NAME)
    # found SDK
    #message("Path VULKAN_SDK = $ENV{VULKAN_SDK}")
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    if (VULKAN_SDK)
        message("found vulkan sdk")
        target_include_directories(${TARGET_NAME} PUBLIC ${VULKAN_SDK}/include)
        target_link_directories(${TARGET_NAME} PUBLIC ${VULKAN_SDK}/lib/)
        target_link_libraries(${TARGET_NAME} PUBLIC ${VKLIB_NAME})
    # not found SDK
    else()
        message("not found vulkan sdk")
        find_package(Vulkan REQUIRED)
        target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})
        target_link_directories(${TARGET_NAME} PUBLIC ${Vulkan_LIBRARIES})
        target_link_libraries(${TARGET_NAME} PUBLIC ${VKLIB_NAME})
    endif()
endfunction(FindVulkan TARGET_NAME VKLIBNAME)

# find package: vulkan
FindVulkan(arctic_engine "vulkan-1.lib")

# find package: vulkan
#[[FetchContent_Declare(external_vulkan
        GIT_REPOSITORY    https://github.com/KhronosGroup/Vulkan-Headers
        GIT_TAG           v1.3.232)
FetchContent_MakeAvailable(external_vulkan)
target_link_libraries(arctic_engine PRIVATE Vulkan::Headers)]]

# find package: glfw
FetchContent_Declare(external_glfw
        GIT_REPOSITORY    https://github.com/glfw/glfw
        GIT_TAG           3.3.8)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(external_glfw)
target_link_libraries(arctic_engine PRIVATE glfw)

# find package: glm
FetchContent_Declare(external_glm
        GIT_REPOSITORY    https://github.com/g-truc/glm
        GIT_TAG           0.9.9.8)
FetchContent_MakeAvailable(external_glm)
target_link_libraries(arctic_engine PRIVATE glm::glm)
