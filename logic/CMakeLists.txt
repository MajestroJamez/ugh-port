cmake_minimum_required(VERSION 3.28)
project(ugh_logic LANGUAGES CXX)

# UGH! game logic: C++20, no dependencies, no exceptions or RTTI in src/ (re/notes/rewrite-design.md).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# the game logic, by module: a folder is a module and a namespace
file(GLOB_RECURSE UGH_LOGIC_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(ugh_logic STATIC ${UGH_LOGIC_SOURCES})
target_include_directories(ugh_logic PUBLIC include src)
if(MSVC)
    # the logic must build as an Unreal Engine module: no exceptions, no RTTI
    target_compile_options(ugh_logic PRIVATE /GR- /EHs-c- /D_HAS_EXCEPTIONS=0)
endif()

# the semantic state as replay fields (UGR 1), for the replay check and the tests
file(GLOB_RECURSE UGH_REPLAY_SOURCES CONFIGURE_DEPENDS replay/*.cpp)
add_library(ugh_logic_replay STATIC ${UGH_REPLAY_SOURCES})
target_include_directories(ugh_logic_replay PUBLIC . src)
target_link_libraries(ugh_logic_replay PUBLIC ugh_logic)

# the logic against the golden replays
file(GLOB UGH_CHECK_SOURCES CONFIGURE_DEPENDS tools/replay_check/*.cpp)
add_executable(replay_check ${UGH_CHECK_SOURCES})
target_link_libraries(replay_check PRIVATE ugh_logic_replay)

set(UGH_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../assets/sim/ugh-data.ugd" CACHE FILEPATH "Game data (.\\gradlew.bat :extractor:run)")
set(UGH_REPLAYS "${CMAKE_CURRENT_SOURCE_DIR}/../verify/build/replays/ugr1" CACHE PATH "Golden replays UGR 1 (.\\gradlew.bat :verify:replays)")
enable_testing()

# tests by module (they need the data)
file(GLOB_RECURSE UGH_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(ugh_logic_tests ${UGH_TEST_SOURCES})
target_link_libraries(ugh_logic_tests PRIVATE ugh_logic_replay)
target_include_directories(ugh_logic_tests PRIVATE tests)
add_test(NAME unit COMMAND ugh_logic_tests "${UGH_DATA}")

# one test per golden replay
file(GLOB UGH_REPLAY_FILES CONFIGURE_DEPENDS "${UGH_REPLAYS}/*.ugr")
if(NOT UGH_REPLAY_FILES)
    message(WARNING "no golden replays in ${UGH_REPLAYS}: run .\\gradlew.bat :verify:replays")
endif()
foreach(replay IN LISTS UGH_REPLAY_FILES)
    get_filename_component(name "${replay}" NAME_WE)
    add_test(NAME "replay.${name}" COMMAND replay_check "${UGH_DATA}" "${replay}")
endforeach()
