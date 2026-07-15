#pragma once
//
// TestFramework.h - tiny header-only micro test framework.
//
// No external dependency is available, so this hand-rolls just enough of
// a unit test framework to support the DataMonitorCore test suite:
//   TEST(name) { ... }              registers a test case
//   CHECK(condition)                records a failure (keeps running) if false
//   CHECK_EQ(a, b)                  records a failure if a != b
//   CHECK_THROWS(expr)              records a failure if expr does not throw
// RunAllTests() executes every registered test and prints a pass/fail
// summary, returning the number of failed tests (0 == success).
//
#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testfw {

struct Failure {
    std::string file;
    int line = 0;
    std::string message;
};

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& Registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline std::vector<Failure>& CurrentFailures() {
    static std::vector<Failure> failures;
    return failures;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn) {
        Registry().push_back(TestCase{name, std::move(fn)});
    }
};

inline void RecordFailure(const char* file, int line, const std::string& message) {
    CurrentFailures().push_back(Failure{file, line, message});
}

inline int RunAllTests() {
    int totalTests = 0;
    int failedTests = 0;

    for (const auto& test : Registry()) {
        ++totalTests;
        CurrentFailures().clear();
        bool threw = false;
        std::string exceptionMessage;
        try {
            test.fn();
        } catch (const std::exception& ex) {
            threw = true;
            exceptionMessage = ex.what();
        } catch (...) {
            threw = true;
            exceptionMessage = "unknown exception";
        }

        if (threw) {
            RecordFailure("<test body>", 0, "unhandled exception: " + exceptionMessage);
        }

        if (CurrentFailures().empty()) {
            std::cout << "[PASS] " << test.name << "\n";
        } else {
            ++failedTests;
            std::cout << "[FAIL] " << test.name << "\n";
            for (const auto& failure : CurrentFailures()) {
                std::cout << "    " << failure.file << ":" << failure.line << ": " << failure.message << "\n";
            }
        }
    }

    std::cout << "\n==============================\n";
    std::cout << "Total: " << totalTests << "  Passed: " << (totalTests - failedTests)
               << "  Failed: " << failedTests << "\n";
    std::cout << "==============================\n";

    return failedTests;
}

} // namespace testfw

#define TESTFW_CONCAT_INNER(a, b) a##b
#define TESTFW_CONCAT(a, b) TESTFW_CONCAT_INNER(a, b)

#define TEST(name)                                                                     \
    static void TESTFW_CONCAT(TestFn_, name)();                                        \
    static ::testfw::Registrar TESTFW_CONCAT(TestReg_, name)(#name, TESTFW_CONCAT(TestFn_, name)); \
    static void TESTFW_CONCAT(TestFn_, name)()

#define CHECK(cond)                                                            \
    do {                                                                       \
        if (!(cond)) {                                                        \
            ::testfw::RecordFailure(__FILE__, __LINE__, "CHECK failed: " #cond); \
        }                                                                      \
    } while (0)

#define CHECK_EQ(a, b)                                                                     \
    do {                                                                                   \
        if (!((a) == (b))) {                                                               \
            std::ostringstream testfw_oss_;                                                \
            testfw_oss_ << "CHECK_EQ failed: " #a " == " #b;                                \
            ::testfw::RecordFailure(__FILE__, __LINE__, testfw_oss_.str());                 \
        }                                                                                   \
    } while (0)

#define CHECK_THROWS(expr)                                                                       \
    do {                                                                                         \
        bool testfw_threw_ = false;                                                              \
        try {                                                                                    \
            (void)(expr);                                                                        \
        } catch (...) {                                                                          \
            testfw_threw_ = true;                                                                \
        }                                                                                         \
        if (!testfw_threw_) {                                                                    \
            ::testfw::RecordFailure(__FILE__, __LINE__, "CHECK_THROWS failed: " #expr " did not throw"); \
        }                                                                                         \
    } while (0)
