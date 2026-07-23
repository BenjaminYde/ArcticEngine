function(compile_shaders_target TARGET)

add_custom_command(
    TARGET ${TARGET}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "Running post-build script..."
    COMMAND bash ${CMAKE_SOURCE_DIR}/scripts/compile_shaders.sh
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running post-build shell script"
)

endfunction()