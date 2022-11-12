# configure target
set(ARCTIC_GAME_VERSION_MAJOR 0)
set(ARCTIC_GAME_VERSION_MINOR 1)
set(ARCTIC_GAME_VERSION_PATCH 0)

# create target
set(TARGET ArcticGame)
message("target is ${TARGET}")
add_executable(${TARGET} game.cpp)

# add module: arctic engine
target_link_libraries(${TARGET} PRIVATE ArcticEngine)

get_target_property(ArcticEngine_INCLUDE_DIRS ArcticEngine INCLUDE_DIRS)
target_include_directories(${TARGET} PRIVATE ${ArcticEngine_INCLUDE_DIRS})
