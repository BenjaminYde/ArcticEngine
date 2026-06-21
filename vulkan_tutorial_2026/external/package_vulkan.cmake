# function to find vulkan
function(FindPackage_Vulkan TARGET_NAME)
    # found SDK
    #message("Path VULKAN_SDK = $ENV{VULKAN_SDK}")
    find_package(Vulkan REQUIRED)
    message("Vulkan FOUND = ${Vulkan_FOUND}")

    add_library(VulkanHppModule)
    target_sources(VulkanHppModule PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS ${Vulkan_INCLUDE_DIR}
        FILES ${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm
    )
    target_compile_definitions(VulkanHppModule PUBLIC
        VULKAN_HPP_NO_SETTERS
        VULKAN_HPP_NO_CONSTRUCTORS
    )
    target_link_libraries(VulkanHppModule PUBLIC Vulkan::Vulkan)

    target_link_libraries(${TARGET_NAME} PRIVATE VulkanHppModule)


    #target_link_libraries(${TARGET_NAME} PRIVATE Vulkan::Vulkan)

    #set(VULKAN_SDK $ENV{VULKAN_SDK})
    #if(DEFINED ENV{VULKAN_SDK})
    #    message(STATUS "Vulkan SDK found: $ENV{VULKAN_SDK}")
    #    target_include_directories(${TARGET_NAME} PRIVATE ${VULKAN_SDK}/include)
    #    target_link_directories(${TARGET_NAME} INTERFACE ${VULKAN_SDK}/lib/)
    #    target_link_libraries(${TARGET_NAME} PRIVATE vulkan)
    #    set(Vulkan_FOUND "True")
    #else()
    #    message(STATUS "Vulkan SDK not found")
    #endif()
endfunction()

