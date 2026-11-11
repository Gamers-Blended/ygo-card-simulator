# Unit tests for the odds engine, using GoogleTest from vcpkg (vcpkg.json)
# Paths in this file are relative to tests/

# CONFIG = use the GTestConfig.cmake file vcpkg installed, which defines the GTest:: targets
find_package(GTest CONFIG REQUIRED)

add_executable(odds_tests
    odds/NormalizeTests.cpp
)
# gtest_main supplies main(), which runs every TEST() in the program
target_link_libraries(odds_tests PRIVATE ygo_odds GTest::gtest_main)

# Registers each TEST() with CTest, so `ctest` can list, run and report them one by one
include(GoogleTest)
gtest_discover_tests(odds_tests)
