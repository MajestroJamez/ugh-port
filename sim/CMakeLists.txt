cmake_minimum_required(VERSION 3.28)
project(ugh_sim LANGUAGES CXX)

# UGH! game logic core: C++20, no dependencies (re/notes/plan.md, steps 5-8).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

add_library(ugh_sim STATIC src/api.cpp src/bonuses.cpp src/data.cpp src/fields.cpp src/game.cpp src/objects.cpp
            src/passengers.cpp)
target_include_directories(ugh_sim PUBLIC include PRIVATE src)

add_executable(ugh_replay tools/ugh_replay.cpp)
target_link_libraries(ugh_replay PRIVATE ugh_sim)

# Two tests per golden replay: the whole game, and every transition on its own (recorded by .\gradlew.bat :verify:replays, data from .\gradlew.bat :extractor:run).
enable_testing()
set(UGH_SIM_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../assets/sim/ugh-sim.bin" CACHE FILEPATH "Data exported by the extractor")
set(UGH_REPLAYS "${CMAKE_CURRENT_SOURCE_DIR}/../verify/build/replays" CACHE PATH "Directory of the golden replays")
file(GLOB UGH_REPLAY_FILES CONFIGURE_DEPENDS "${UGH_REPLAYS}/*.ugr")
if(NOT UGH_REPLAY_FILES)
    message(WARNING "no golden replays in ${UGH_REPLAYS}: run .\\gradlew.bat :verify:replays")
endif()
foreach(replay IN LISTS UGH_REPLAY_FILES)
    get_filename_component(name "${replay}" NAME_WE)
    add_test(NAME "replay.${name}" COMMAND ugh_replay "${UGH_SIM_DATA}" "${replay}")
    add_test(NAME "replay-each.${name}" COMMAND ugh_replay --each "${UGH_SIM_DATA}" "${replay}")
endforeach()
