// A minimal test framework: the only macros of the project.
//
//   TEST(passengerRidesAndPays) {        // a test: a function, registered by its name
//       CHECK(copter.hasRoom());         // a failed check is printed with its file and line; the test goes on
//   }
//
// Every test file registers its tests; TestMain.cpp loads the game data and runs them all. Macros are needed only
// for two things a function cannot do: to register a test before main() and to print the checked expression.
#pragma once

#include "data/GameData.hpp"

namespace ugh::test {

using TestFunction = void (*)();

/** Registers a test; TEST does it. */
class Registration {
public:
    Registration(const char* name, TestFunction test);
};

/** Counts a failed CHECK; CHECK does it. */
void fail(const char* file, int line, const char* expression);

/** The game data the tests run on (assets/sim/ugh-sim.bin). */
const data::GameData& gameData();

}  // namespace ugh::test

#define TEST(name)                                                                \
    static void name();                                                           \
    static const ugh::test::Registration name##Registration(#name, name);         \
    static void name()

#define CHECK(condition)                                                          \
    do {                                                                          \
        if (!(condition)) ugh::test::fail(__FILE__, __LINE__, #condition);        \
    } while (false)
