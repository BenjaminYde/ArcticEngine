function(EnableClangTidy ARG_VALUE)
    # set the value
    option(ENABLE_CLANG_TIDY "Run clang-tidy as part of the build" ${ARG_VALUE})
    if(NOT ENABLE_CLANG_TIDY)
        return()
    endif()

    # find the program
    find_program(CLANG_TIDY_EXE NAMES clang-tidy-22 clang-tidy)
    if(NOT CLANG_TIDY_EXE)
        message(STATUS "clang-tidy: not found, skipping build-time checks")
        return()
    endif()

    message(STATUS "clang-tidy: enabled (${CLANG_TIDY_EXE})")
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}" PARENT_SCOPE)
endfunction()