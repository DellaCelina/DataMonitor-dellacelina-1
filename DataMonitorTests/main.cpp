#include "TestFramework.h"

int main() {
    int failed = testfw::RunAllTests();
    return failed == 0 ? 0 : 1;
}
