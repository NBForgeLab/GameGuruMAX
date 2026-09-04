// Entry point for the Wicked contract tests: cross-repository integration
// tests that live on the GameGuru MAX side but exercise the real Wicked
// Engine headers and the real wickedcalls.cpp translation layer.
// doctest turns any failed test into a non-zero exit code for the gate.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
