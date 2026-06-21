# create target
set(TARGET Utilities)
message("target is ${TARGET}")
add_library(${TARGET} STATIC)

# set variables
set(INCLUDE_DIR ${CMAKE_SOURCE_DIR}/include)
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR})

# set properties
set_target_properties(
        ${TARGET} 
        PROPERTIES
        INCLUDE_DIR ${INCLUDE_DIR}
)

# set sources
target_sources(
        ${TARGET}
        PUBLIC
        ${INCLUDE_DIR}/arctic/core/utilities/file_utility.h
        ${INCLUDE_DIR}/arctic/core/utilities/application.h
        PRIVATE
        ${SRC_DIR}/file_utility.cpp
)

# set includes
target_include_directories(
        ${TARGET}
        PRIVATE
        ${INCLUDE_DIR}
)