# function to find glfw
function(FindPackage_SDL TARGET_NAME)

    find_package(SDL3 REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3)
    message(STATUS "SDL3 FOUND: ${SDL3_VERSION}")
endfunction()