// A minimal test framework: TEST registers a test, CHECK / CHECK_EQUAL report failures.
#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace ugh::test {

/** A registered test. */
struct TestCase {
    const char* name;
    void (*run)();
};

/** All tests of the program, and the failures of the one running. */
class Registry {
public:
    static Registry& instance() {
        static Registry registry;
        return registry;
    }
    void add(TestCase test) { tests_.push_back(test); }
    const std::vector<TestCase>& tests() const { return tests_; }
    void fail(const std::string& text) {
        failures_++;
        std::cout << "  FAIL " << text << "\n";
    }
    int failures() const { return failures_; }

private:
    std::vector<TestCase> tests_;
    int failures_ = 0;
};

/** Registers a test at static initialization. */
struct Registrar {
    Registrar(const char* name, void (*run)()) { Registry::instance().add({name, run}); }
};

/** The path of the game data, from the command line. */
std::string& dataPath();

}  // namespace ugh::test

#define TEST(name)                                                         \
    static void name();                                                    \
    static ugh::test::Registrar name##_registrar(#name, name);             \
    static void name()

#define CHECK(condition)                                                                      \
    do {                                                                                      \
        if (!(condition)) ugh::test::Registry::instance().fail(std::string(__FILE__) + ":" +  \
                                                               std::to_string(__LINE__) +     \
                                                               ": " #condition);              \
    } while (false)

#define CHECK_EQUAL(expected, actual)                                                                  \
    do {                                                                                               \
        auto ugh_e = (expected);                                                                       \
        auto ugh_a = (actual);                                                                         \
        if (!(ugh_e == ugh_a)) {                                                                       \
            std::ostringstream ugh_s;                                                                  \
            ugh_s << __FILE__ << ":" << __LINE__ << ": " #actual " is " << ugh_a << ", expected " << ugh_e; \
            ugh::test::Registry::instance().fail(ugh_s.str());                                         \
        }                                                                                              \
    } while (false)
