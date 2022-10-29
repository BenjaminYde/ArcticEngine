# configure cmake
cmake_minimum_required(VERSION 3.24)
include(FetchContent)

# configure project
set(CMAKE_CXX_STANDARD 20)
project(arctic_engine LANGUAGES "CXX")
add_executable(arctic_engine main.cpp)

# function to find vulkan
function(FindVulkan TARGET_NAME VKLIB_NAME)
    # found SDK
    #message("Path VULKAN_SDK = $ENV{VULKAN_SDK}")
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    if (VULKAN_SDK)
        #message("found sdk")
        target_include_directories(${TARGET_NAME} PUBLIC ${VULKAN_SDK}/include)
        target_link_directories(${TARGET_NAME} PUBLIC ${VULKAN_SDK}/lib/)
        target_link_libraries(${TARGET_NAME} PUBLIC ${VKLIB_NAME})
    # not found SDK
    else()
        #message("not found sdk")
        find_package(Vulkan REQUIRED)
        target_include_directories(${TARGET_NAME} PUBLIC ${Vulkan_INCLUDE_DIR})
        target_link_directories(${TARGET_NAME} PUBLIC ${Vulkan_LIBRARIES})
        target_link_libraries(${TARGET_NAME} PUBLIC ${VKLIB_NAME})
    endif()
endfunction(FindVulkan TARGET_NAME VKLIBNAME)

# find packages
FindVulkan(arctic_engine "vulkan-1.lib")

# find package: glm
FetchContent_Declare(external_glm
        GIT_REPOSITORY    https://github.com/g-truc/glm
        GIT_TAG           0.9.9.8)
FetchContent_MakeAvailable(external_glm)
target_link_libraries(arctic_engine PRIVATE glm::glm)
