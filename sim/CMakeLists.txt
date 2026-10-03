cmake_minimum_required(VERSION 3.28)
project(ugh_sim LANGUAGES CXX)

# UGH! game logic core: C++20, no dependencies (re/notes/plan.md, steps 5-9; re/notes/core-design.md).
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra)
endif()

# the sources by module: a folder is a module and a namespace (ugh::core, ugh::data ...)
set(UGH_CORE src/core/Random.cpp)
set(UGH_DATA src/data/CollisionMask.cpp src/data/GameData.cpp src/data/GameDataLoader.cpp)
set(UGH_MODEL src/model/BonusItem.cpp src/model/BonusSlots.cpp src/model/Copter.cpp src/model/Enemy.cpp
    src/model/GameSession.cpp src/model/Level.cpp src/model/Passenger.cpp src/model/Rain.cpp src/model/Water.cpp)
set(UGH_PHYSICS src/physics/CollisionProbe.cpp src/physics/CopterPhysics.cpp src/physics/TouchBox.cpp)
set(UGH_PASSENGERS src/passengers/Gone.cpp src/passengers/PassengerState.cpp src/passengers/PassengerStates.cpp
    src/passengers/walking/Appearing.cpp src/passengers/walking/Arriving.cpp src/passengers/walking/Boarding.cpp
    src/passengers/walking/Calling.cpp src/passengers/walking/Entering.cpp src/passengers/walking/Impatient.cpp
    src/passengers/walking/NextStop.cpp src/passengers/walking/OnPickupPad.cpp src/passengers/walking/Riding.cpp
    src/passengers/walking/Waiting.cpp src/passengers/walking/WalkingAway.cpp
    src/passengers/swimming/Sinking.cpp src/passengers/swimming/Splash.cpp src/passengers/swimming/SwimBoarding.cpp
    src/passengers/swimming/SwimCalling.cpp src/passengers/swimming/SwimWaving.cpp src/passengers/swimming/Swimming.cpp
    src/passengers/standing/Falling.cpp src/passengers/standing/Hanging.cpp src/passengers/standing/Standing.cpp
    src/passengers/standing/StartStanding.cpp)
set(UGH_ENEMIES src/enemies/EnemyBehavior.cpp src/enemies/EnemyState.cpp src/enemies/EnemyStates.cpp
    src/enemies/flyer/FlyerBehavior.cpp src/enemies/flyer/FlyerFalling.cpp src/enemies/flyer/FlyerInit.cpp
    src/enemies/flyer/FlyerWait.cpp src/enemies/flyer/FlyerWait2.cpp src/enemies/flyer/Flying.cpp
    src/enemies/walker/Charging.cpp src/enemies/walker/Recovering.cpp src/enemies/walker/Stunned.cpp
    src/enemies/walker/WalkerBehavior.cpp src/enemies/walker/WalkerInit.cpp src/enemies/walker/WalkerState.cpp
    src/enemies/walker/Walking.cpp src/enemies/walker/Watching.cpp
    src/enemies/blower/BlowerBehavior.cpp src/enemies/blower/BlowerInit.cpp src/enemies/blower/BlowerWait.cpp
    src/enemies/blower/Blowing.cpp
    src/enemies/tree/Inactive.cpp src/enemies/tree/TreeBehavior.cpp src/enemies/tree/TreeInit.cpp
    src/enemies/tree/TreeSwaying.cpp src/enemies/tree/TreeWait.cpp)
set(UGH_BONUSES src/bonuses/BonusState.cpp src/bonuses/BonusStates.cpp src/bonuses/Falling.cpp src/bonuses/Lying.cpp)
set(UGH_INPUT src/input/Keyboard.cpp)
set(UGH_GAME src/game/Game.cpp src/game/GameFlow.cpp src/game/LevelLoader.cpp src/game/PlayFrame.cpp
    src/game/phases/CaptionFadeIn.cpp src/game/phases/CaptionFadeOut.cpp
    src/game/phases/CaptionWaitKey.cpp src/game/phases/Playing.cpp src/game/phases/TimedPhase.cpp)
set(UGH_REPLAY src/replay/BonusFields.cpp src/replay/CopterFields.cpp src/replay/EnemyFields.cpp src/replay/FieldReader.cpp
    src/replay/FieldVisitor.cpp src/replay/FieldWriter.cpp src/replay/GameFields.cpp src/replay/PadFields.cpp
    src/replay/PassengerFields.cpp src/replay/ReplayProjection.cpp src/replay/ReplayText.cpp)
set(UGH_API src/api/SimApi.cpp)

add_library(ugh_sim STATIC ${UGH_CORE} ${UGH_DATA} ${UGH_MODEL} ${UGH_PHYSICS} ${UGH_PASSENGERS} ${UGH_ENEMIES}
            ${UGH_BONUSES} ${UGH_INPUT} ${UGH_GAME} ${UGH_REPLAY} ${UGH_API})
target_include_directories(ugh_sim PUBLIC include PRIVATE src)

# The replay player: the C++ core against the golden replays, through the C API only.
add_executable(ugh_replay tools/ugh_replay/main.cpp tools/ugh_replay/ReplayFile.cpp tools/ugh_replay/ReplayPlayer.cpp
               tools/ugh_replay/ReplayReport.cpp tools/ugh_replay/TwinCores.cpp)
target_link_libraries(ugh_replay PRIVATE ugh_sim)

set(UGH_SIM_DATA "${CMAKE_CURRENT_SOURCE_DIR}/../assets/sim/ugh-sim.bin" CACHE FILEPATH "Data exported by the extractor")
set(UGH_REPLAYS "${CMAKE_CURRENT_SOURCE_DIR}/../verify/build/replays" CACHE PATH "Directory of the golden replays")
enable_testing()

# Unit tests by module, next to the sources they test (they need the data).
add_executable(ugh_sim_tests tests/TestMain.cpp tests/LevelSetup.cpp
    tests/core/CountdownTest.cpp tests/core/FixedTest.cpp tests/core/RandomTest.cpp tests/core/SpeedTest.cpp
    tests/core/WordTest.cpp
    tests/data/CollisionMaskTest.cpp tests/data/GameDataLoaderTest.cpp
    tests/model/WaterTest.cpp
    tests/physics/CollisionProbeTest.cpp tests/physics/CopterPhysicsTest.cpp tests/physics/TouchBoxTest.cpp
    tests/input/KeyboardTest.cpp
    tests/passengers/WalkingPassengerTest.cpp
    tests/bonuses/BonusTest.cpp
    tests/game/GameFlowTest.cpp)
target_link_libraries(ugh_sim_tests PRIVATE ugh_sim)
target_include_directories(ugh_sim_tests PRIVATE src tests)
add_test(NAME unit COMMAND ugh_sim_tests "${UGH_SIM_DATA}")

# Two tests per golden replay: the whole game, and every transition on its own (recorded by .\gradlew.bat :verify:replays, data from .\gradlew.bat :extractor:run).
file(GLOB UGH_REPLAY_FILES CONFIGURE_DEPENDS "${UGH_REPLAYS}/*.ugr")
if(NOT UGH_REPLAY_FILES)
    message(WARNING "no golden replays in ${UGH_REPLAYS}: run .\\gradlew.bat :verify:replays")
endif()
foreach(replay IN LISTS UGH_REPLAY_FILES)
    get_filename_component(name "${replay}" NAME_WE)
    add_test(NAME "replay.${name}" COMMAND ugh_replay "${UGH_SIM_DATA}" "${replay}")
    add_test(NAME "replay-each.${name}" COMMAND ugh_replay --each "${UGH_SIM_DATA}" "${replay}")
endforeach()
