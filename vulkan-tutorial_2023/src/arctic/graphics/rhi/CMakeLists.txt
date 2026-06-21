# configure target
#set(ARCTIC_RENDERING_VERSION_MAJOR 0)
#set(ARCTIC_RENDERING_VERSION_MINOR 1)
#set(ARCTIC_RENDERING_VERSION_PATCH 0)

# create target
set(TARGET ARCTIC_GRAPHICS_RHI)
message("target is ${TARGET}")
add_library(${TARGET} STATIC)
set_target_properties(${TARGET} PROPERTIES LINKER_LANGUAGE CXX)

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
    ${INCLUDE_DIR}/arctic/graphics/rhi/uniform_buffer_object.h
    ${INCLUDE_DIR}/arctic/graphics/rhi/vertex.h
)

# set includes
target_include_directories(
    ${TARGET}
    PRIVATE
    ${INCLUDE_DIR}
)

# link packages
FindPackage_GLM(${TARGET})