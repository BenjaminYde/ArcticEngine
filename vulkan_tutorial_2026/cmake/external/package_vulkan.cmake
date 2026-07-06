# function to find vulkan
function(FindPackage_Vulkan TARGET_NAME)
    # find package
    find_package(Vulkan REQUIRED)
    if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan NOT Found!")
    endif()
    message(STATUS "Vulkan FOUND: ${Vulkan_VERSION}")
    #message("Path VULKAN_SDK = $ENV{VULKAN_SDK}")

    # create library
    add_library(VulkanHppModule INTERFACE)
    target_include_directories(VulkanHppModule INTERFACE
        ${Vulkan_INCLUDE_DIR}
    )
    target_compile_definitions(VulkanHppModule INTERFACE
        VULKAN_HPP_NO_SETTERS
        VULKAN_HPP_NO_CONSTRUCTORS
    )
    target_link_libraries(VulkanHppModule INTERFACE Vulkan::Vulkan)

    # link libraries to target
    target_link_libraries(${TARGET_NAME} PRIVATE VulkanHppModule)
endfunction()

