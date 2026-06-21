# configure target
set(ARCTIC_GAME_VERSION_MAJOR 0)
set(ARCTIC_GAME_VERSION_MINOR 1)
set(ARCTIC_GAME_VERSION_PATCH 0)

# create target
set(TARGET ArcticGame)
message("target is ${TARGET}")
add_executable(${TARGET} game.cpp)

# add module: arctic engine
target_link_libraries(${TARGET} PRIVATE ARCTIC_CORE_ENGINE)
get_target_property(ARCTIC_CORE_ENGINE_INCLUDE_DIR ARCTIC_CORE_ENGINE INCLUDE_DIR)
target_include_directories(${TARGET} PRIVATE ${ARCTIC_CORE_ENGINE_INCLUDE_DIR})