# function to find vulkan
function(FindPackage_GLFW TARGET_NAME)
    FetchContent_Declare(external_glfw
            GIT_REPOSITORY    https://github.com/glfw/glfw
            GIT_TAG           3.3.8)

    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(external_glfw)

    target_link_libraries(${TARGET_NAME} PRIVATE glfw)
    target_include_directories(${TARGET_NAME} PRIVATE ${external_glfw_SOURCE_DIR}/include)

endfunction()