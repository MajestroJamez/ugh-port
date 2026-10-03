cmake_minimum_required(VERSION 3.28)
project(ugh_logic LANGUAGES CXX)

# UGH! game logic: C++20, no dependencies, no exceptions or RTTI in src/ (docs/rewrite-design.md).
# The golden replays are not checked here but in 6_verification/, which adds this folder.
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
# the logic must build as an Unreal Engine module: no exceptions, no RTTI
if(MSVC)
    target_compile_options(ugh_logic PRIVATE /GR- /EHs-c- /D_HAS_EXCEPTIONS=0)
else()
    target_compile_options(ugh_logic PRIVATE -fno-exceptions -fno-rtti)
endif()

# the test pilot of the replays (I lines): for the tests and 6_verification, not part of the logic
add_library(ugh_logic_testing STATIC testing/TestPilot.cpp)
target_include_directories(ugh_logic_testing PUBLIC .)   # "testing/TestPilot.hpp"
target_link_libraries(ugh_logic_testing PUBLIC ugh_logic)

set(UGH_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../../assets/logic/ugh-data.ugd" CACHE FILEPATH "Game data (.\\gradlew.bat :extractor:run)")
enable_testing()

# tests by module (they need the data)
file(GLOB_RECURSE UGH_TEST_SOURCES CONFIGURE_DEPENDS tests/*.cpp)
add_executable(ugh_logic_tests ${UGH_TEST_SOURCES})
target_link_libraries(ugh_logic_tests PRIVATE ugh_logic_testing)
target_include_directories(ugh_logic_tests PRIVATE tests)
add_test(NAME unit COMMAND ugh_logic_tests "${UGH_DATA}")
