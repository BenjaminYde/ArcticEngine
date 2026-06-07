# configure target
set(ARCTIC_ENGINE_VERSION_MAJOR 0)
set(ARCTIC_ENGINE_VERSION_MINOR 1)
set(ARCTIC_ENGINE_VERSION_PATCH 0)

# create target
set(TARGET ARCTIC_CORE_ENGINE)
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
        PRIVATE
        ${SRC_DIR}/arctic_engine.cpp
)

# set includes
target_include_directories(
        ${TARGET}
        PRIVATE
        ${INCLUDE_DIR}
)

FindPackage_SDL(${TARGET})
#FindPackage_Vulkan(${TARGET})
#FindPackage_GLM(${TARGET})

# add module: arctic vulkan
target_link_libraries(
        ${TARGET} 
        PRIVATE 
        ARCTIC_GRAPHICS_VULKAN)

get_target_property(
        ARCTIC_GRAPHICS_VULKAN_INCLUDE_DIR
        ARCTIC_GRAPHICS_VULKAN
        INCLUDE_DIR)

target_include_directories(
        ${TARGET} 
        PRIVATE 
        ${ARCTIC_GRAPHICS_VULKAN_INCLUDE_DIR})

# add module: arctic rendering
target_link_libraries(
        ${TARGET} 
        PRIVATE 
        ARCTIC_GRAPHICS_RHI)

get_target_property(
        ARCTIC_GRAPHICS_RHI_INCLUDE_DIR
        ARCTIC_GRAPHICS_RHI
        INCLUDE_DIR)

target_include_directories(
        ${TARGET} 
        PRIVATE 
        ${ARCTIC_GRAPHICS_RHI_INCLUDE_DIR})