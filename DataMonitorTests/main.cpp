#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "TestFramework.h"

int main() {
#ifdef _WIN32
    // Some test cases contain Korean string literals (compiled with /utf-8,
    // so they're UTF-8 bytes); switch the console so failure output renders
    // correctly instead of garbling under the default system codepage.
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    int failed = testfw::RunAllTests();
    return failed == 0 ? 0 : 1;
}
