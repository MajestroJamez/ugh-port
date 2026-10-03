// Runs every test: ugh_logic_tests <ugh-data.ugd>
#include "TestFramework.hpp"

std::string& ugh::test::dataPath() {
    static std::string path;
    return path;
}

int main(int argc, char** argv) {
    if (argc > 1) ugh::test::dataPath() = argv[1];
    auto& registry = ugh::test::Registry::instance();
    for (const auto& test : registry.tests()) {
        int before = registry.failures();
        test.run();
        std::cout << (registry.failures() == before ? "ok   " : "FAIL ") << test.name << "\n";
    }
    std::cout << registry.tests().size() << " tests, " << registry.failures() << " failures\n";
    return registry.failures() == 0 ? 0 : 1;
}
