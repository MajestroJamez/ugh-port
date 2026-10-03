// Unit tests of the core's value types, services and state machines, next to the golden replays (which check
// everything against the original). Usage: ugh_sim_tests <ugh-sim.bin>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "data/GameDataLoader.hpp"

namespace ugh::test {

namespace {

struct Test {
    const char* name;
    TestFunction run;
};

/** The registered tests (a function, so that it exists before the registrations of the other files run). */
std::vector<Test>& tests() {
    static std::vector<Test> all;
    return all;
}

int failures = 0;
std::unique_ptr<const data::GameData> loaded;

}  // namespace

Registration::Registration(const char* name, TestFunction test) { tests().push_back({name, test}); }

void fail(const char* file, int line, const char* expression) {
    std::printf("  %s:%d: CHECK(%s) failed\n", file, line, expression);
    failures++;
}

const data::GameData& gameData() { return *loaded; }

}  // namespace ugh::test

int main(int argc, char** argv) {
    using namespace ugh::test;
    if (argc < 2) {
        std::fprintf(stderr, "usage: ugh_sim_tests <ugh-sim.bin>\n");
        return 2;
    }
    std::string error;
    loaded = ugh::data::GameDataLoader::load(argv[1], error);
    if (!loaded) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    for (const Test& test : tests()) {
        int before = failures;
        test.run();
        std::printf("%s %s\n", failures == before ? "ok  " : "FAIL", test.name);
    }
    std::printf("%zu tests, %d failures\n", tests().size(), failures);
    return failures == 0 ? 0 : 1;
}
