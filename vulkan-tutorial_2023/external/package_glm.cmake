# function to find glm
function(FindPackage_GLM TARGET_NAME)
    FetchContent_Declare(external_glm
            GIT_REPOSITORY    https://github.com/g-truc/glm
            GIT_TAG           1.0.3
            GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(external_glm)
    message(STATUS "GLM: 1.0.3")
    target_link_libraries(${TARGET_NAME} PRIVATE glm::glm)
endfunction()