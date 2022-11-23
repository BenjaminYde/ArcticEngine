# create target
set(TARGET Utilities)
message("target is ${TARGET}")
add_library(${TARGET} STATIC)

# set variables
set(INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(INCLUDE_DIRS_INTERNAL ${INCLUDE_DIRS}/utilities)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

# set properties
set_target_properties(${TARGET} PROPERTIES
        INCLUDE_DIRS ${INCLUDE_DIRS})

# set sources
target_sources(${TARGET}
        PUBLIC
        ${INCLUDE_DIRS_INTERNAL}/file_utility.h
        PRIVATE
        ${SRC_DIR}/file_utility.cpp)

# set includes
target_include_directories(${TARGET}
        PRIVATE
        ${INCLUDE_DIRS})