// GameGuru MAX - End-to-End and System Integration Test Suite
// Entry point powered by doctest v2.4.12
#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    std::cout << "===================================================================\n";
    std::cout << " GameGuru MAX Modernization - End-to-End Test Suite (Tiers 1 - 4)  \n";
    std::cout << " Opaque-Box Verification & Legacy Architecture Excision Gate       \n";
    std::cout << "===================================================================\n";

    doctest::Context context(argc, argv);
    const int result = context.run();

    if (result == 0)
    {
        std::cout << "\n[GameGuruE2ETests] ALL TEST SUITES PASSED CLEANLY.\n";
    }
    else
    {
        std::cout << "\n[GameGuruE2ETests] TEST SUITE REPORTED FAILURES (exit code: " << result << ").\n";
    }

    return result;
}
