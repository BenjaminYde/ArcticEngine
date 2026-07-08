function(FindPackageTarget_stblog TARGET)
    # import modules
    include(FetchContent)

    # fetch
    message(STATUS "spdlog: fetching from Github")
    FetchContent_Declare(external_spdlog
        GIT_REPOSITORY git@github.com:gabime/spdlog.git
        GIT_TAG v1.17.0
        GIT_SHALLOW TRUE
        SYSTEM
    )
    FetchContent_MakeAvailable(external_spdlog)

    # link libraries
    target_link_libraries(${TARGET} PRIVATE spdlog::spdlog)
endfunction()