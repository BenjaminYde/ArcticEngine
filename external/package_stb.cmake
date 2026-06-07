# function to find glm
function(FindPackage_STB TARGET_NAME)
    FetchContent_Declare(external_stb
            GIT_REPOSITORY    https://github.com/nothings/stb)

    FetchContent_MakeAvailable(external_stb)

    target_include_directories(${TARGET_NAME} PRIVATE ${external_stb_SOURCE_DIR})
endfunction()