# configure target
#set(ARCTIC_VULKAN_VERSION_MAJOR 0)
#set(ARCTIC_VULKAN_VERSION_MINOR 1)
#set(ARCTIC_VULKAN_VERSION_PATCH 0)

# create target
set(TARGET ARCTIC_GRAPHICS_VULKAN)
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
target_sources(${TARGET}
        PRIVATE
        ${SRC_DIR}/vk_context.cpp
        ${SRC_DIR}/vk_renderloop.cpp
        ${SRC_DIR}/vk_renderpipeline.cpp
        ${SRC_DIR}/vk_swapchain.cpp
        ${SRC_DIR}/vk_loader.cpp
        ${SRC_DIR}/vk_window.cpp
        ${SRC_DIR}/vk_memory_handler.cpp
        ${SRC_DIR}/render_utils.cpp
)

# set includes
target_include_directories(
        ${TARGET}
        PRIVATE
        ${INCLUDE_DIR}
)

# link packages
FindPackage_Vulkan(${TARGET})
FindPackage_SDL(${TARGET})
FindPackage_GLM(${TARGET})
FindPackage_STB(${TARGET})
FindPackage_VK_MEM_ALLOC(${TARGET})

# add module: x11
find_package(X11 REQUIRED)
target_link_libraries(${TARGET} PRIVATE X11::X11_xcb)

# add module: utilities
target_link_libraries(
        ${TARGET}
        PRIVATE
        Utilities
        X11::X11_xcb
)

get_target_property(
        Utilties_INCLUDE_DIR
        Utilities
        INCLUDE_DIR
)

target_include_directories(
        ${TARGET}
        PRIVATE
        ${Utilties_INCLUDE_DIR}
)

# add module: arctic graphics rhi
target_link_libraries(
        ${TARGET}
        PRIVATE
        ARCTIC_GRAPHICS_RHI
)

get_target_property(
        ARCTIC_GRAPHICS_RHI_INCLUDE_DIR
        ARCTIC_GRAPHICS_RHI
        INCLUDE_DIR
)

target_include_directories(
        ${TARGET}
        PRIVATE
        ${ARCTIC_GRAPHICS_RHI_INCLUDE_DIR}
)