# function to find vulkan
function(FindPackage_GLM TARGET_NAME)
    FetchContent_Declare(external_glm
            GIT_REPOSITORY    https://github.com/g-truc/glm
            GIT_TAG           0.9.9.8)

    FetchContent_MakeAvailable(external_glm)

    target_link_libraries(${TARGET} PRIVATE glm::glm)
endfunction()