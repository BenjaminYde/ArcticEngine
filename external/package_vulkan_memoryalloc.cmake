function(FindPackage_VK_MEM_ALLOC TARGET_NAME)
    FetchContent_Declare(external_vk_mem_alloc
    GIT_REPOSITORY    https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator
    GIT_TAG           v3.4.0)

    FetchContent_MakeAvailable(external_vk_mem_alloc)

    #find_package(VulkanMemoryAllocator REQUIRED)
    target_link_libraries(${TARGET_NAME} PRIVATE GPUOpen::VulkanMemoryAllocator)

endfunction()