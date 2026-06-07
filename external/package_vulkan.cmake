# function to find vulkan
function(FindPackage_Vulkan TARGET_NAME)
    # found SDK
    #message("Path VULKAN_SDK = $ENV{VULKAN_SDK}")
    set(VULKAN_SDK $ENV{VULKAN_SDK})
    if(DEFINED ENV{VULKAN_SDK})
        message(STATUS "Vulkan SDK found: $ENV{VULKAN_SDK}")
        target_include_directories(${TARGET_NAME} PRIVATE ${VULKAN_SDK}/include)
        target_link_directories(${TARGET_NAME} INTERFACE ${VULKAN_SDK}/lib/)
        target_link_libraries(${TARGET_NAME} PRIVATE vulkan)
        set(Vulkan_FOUND "True")
        # not found SDK
    else()
        message(STATUS "Vulkan SDK not found")
        find_package(Vulkan REQUIRED)
        target_include_directories(${TARGET_NAME} PRIVATE ${Vulkan_INCLUDE_DIR})
        target_link_directories(${TARGET_NAME} PRIVATE ${Vulkan_LIBRARIES})
        target_link_libraries(${TARGET_NAME} PRIVATE vulkan-1.lib)
    endif()

    # final message
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "Could not find Vulkan library!")
    endif()

    # find package: vulkan
    #[[FetchContent_Declare(external_vulkan
            GIT_REPOSITORY    https://github.com/KhronosGroup/Vulkan-Headers
            GIT_TAG           v1.3.232)
    FetchContent_MakeAvailable(external_vulkan)
    target_link_libraries(${TARGET_ENGINE} PRIVATE Vulkan::Headers)]]

endfunction()

