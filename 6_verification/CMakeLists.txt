cmake_minimum_required(VERSION 3.28)
project(ugh_verification LANGUAGES CXX)

# The game logic of 5_remake/logic against the golden replays of 4_test_data/verify, field by field.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

enable_testing()

# the logic with its unit tests
add_subdirectory(../5_remake/logic logic)

# the semantic state as replay fields (UGR 1); it only reads the game
file(GLOB_RECURSE UGH_REPLAY_SOURCES CONFIGURE_DEPENDS replay/*.cpp)
add_library(ugh_logic_replay STATIC ${UGH_REPLAY_SOURCES})
target_include_directories(ugh_logic_replay PUBLIC .)
target_link_libraries(ugh_logic_replay PUBLIC ugh_logic)

# the logic against the golden replays
file(GLOB UGH_CHECK_SOURCES CONFIGURE_DEPENDS tools/replay_check/*.cpp)
add_executable(replay_check ${UGH_CHECK_SOURCES})
target_link_libraries(replay_check PRIVATE ugh_logic_replay)

set(UGH_REPLAYS "${CMAKE_CURRENT_SOURCE_DIR}/../4_test_data/verify/build/replays" CACHE PATH "Golden replays UGR 1 (.\\gradlew.bat :verify:replays)")

# one test per golden replay
file(GLOB UGH_REPLAY_FILES CONFIGURE_DEPENDS "${UGH_REPLAYS}/*.ugr")
if(NOT UGH_REPLAY_FILES)
    message(WARNING "no golden replays in ${UGH_REPLAYS}: run .\\gradlew.bat :verify:replays")
endif()
foreach(replay IN LISTS UGH_REPLAY_FILES)
    get_filename_component(name "${replay}" NAME_WE)
    add_test(NAME "replay.${name}" COMMAND replay_check "${UGH_DATA}" "${replay}")
endforeach()
