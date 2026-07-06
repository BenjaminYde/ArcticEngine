# function to find sdl3
function(FindPackage_SDL TARGET_NAME)
    # find package
    find_package(SDL3 REQUIRED)
    if (NOT SDL3_FOUND)
         message(FATAL_ERROR "SDL3 NOT Found!")
    endif()
    message(STATUS "SDL3 FOUND: ${SDL3_VERSION}")

    # link library
    target_link_libraries(${TARGET_NAME} PRIVATE SDL3::SDL3)
endfunction()