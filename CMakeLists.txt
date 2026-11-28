cmake_minimum_required(VERSION 3.18) # 3.18: gtest_discover_tests(DISCOVERY_MODE PRE_TEST)

# vcpkg supplies third-party libraries (GoogleTest, nlohmann/json) listed in vcpkg.json
# Toolchain file has to be set before project() because project() is where CMake sets up compiler
# Qt itself still comes from Qt installer, not from vcpkg
if(DEFINED ENV{VCPKG_ROOT} AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "vcpkg toolchain")
endif()

# Windows: build vcpkg libraries as static libs but use DLL C runtime (/MD), same runtime Qt's MSVC build uses
# Mixing runtimes causes linker errors
# Linux's default triplet (x64-linux) is already static
if(WIN32 AND NOT DEFINED VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static-md" CACHE STRING "vcpkg triplet")
endif()

# ON by default so a plain configure builds the tests; pass -DYGO_BUILD_TESTS=OFF to skip them
# (and skip downloading GoogleTest)
option(YGO_BUILD_TESTS "Build unit tests" ON)
if(YGO_BUILD_TESTS)
    # Asks vcpkg to also install the "tests" feature from vcpkg.json (gtest)
    # Has to be set before project(), because that's when vcpkg installs packages
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

project(ygoCardSimulator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) # Compile with C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON) # If compiler can't do C++20, CMake stops with an error instead of quietly using older version
set(CMAKE_CXX_EXTENSIONS OFF) # Strict standard C++20 (-std=c++20, not gnu++20) so compiler-specific extensions don't sneak in and break MSVC builds

# Compiler warnings for our own code, as a target to link PRIVATE: only targets that link it get the flags,
# so Qt, vcpkg libraries and Qt-generated files aren't affected
add_library(ygo_warnings INTERFACE)
if(MSVC)
    target_compile_options(ygo_warnings INTERFACE /W4)
else() # GCC and Clang
    target_compile_options(ygo_warnings INTERFACE -Wall -Wextra -Wpedantic)
endif()

# JSON parsing for per-set configs (data/*.json), installed by vcpkg (vcpkg.json)
find_package(nlohmann_json CONFIG REQUIRED)

# Odds engine: plain C++, deliberately declared before Qt is found so it can't depend on it
# STATIC = compiled once into a library archive that gets copied into each program linking it
# Headers are listed so IDEs like Qt Creator show them under target
add_library(ygo_odds STATIC
    src/odds/ConfigError.h
    src/odds/ConfigLoader.cpp
    src/odds/ConfigLoader.h
    src/odds/Normalize.cpp
    src/odds/Normalize.h
    src/odds/PackConfig.h
    src/odds/PackLayout.cpp
    src/odds/PackLayout.h
    src/odds/PackOpener.cpp
    src/odds/PackOpener.h
    src/odds/RawSlotTable.h
)
# PUBLIC: ygo_odds' own .cpp files and anything linking ygo_odds can write #include "odds/..."
target_include_directories(ygo_odds PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
# PRIVATE: warnings apply when compiling ygo_odds itself, they aren't passed on to whatever links it
target_link_libraries(ygo_odds PRIVATE ygo_warnings)
# PRIVATE: only ConfigLoader.cpp includes the JSON header, none of ygo_odds' public headers do,
# so programs linking ygo_odds don't get (or need) the JSON include path
target_link_libraries(ygo_odds PRIVATE nlohmann_json::nlohmann_json)

# Console demo: prints simulated pulls, no UI
add_executable(odds_demo src/odds_demo/main.cpp)
target_link_libraries(odds_demo PRIVATE ygo_odds ygo_warnings)

# Unit tests live in tests/CMakeLists.txt
# enable_testing() has to come first, so `ctest` run from the build folder finds them
if(YGO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

find_package(Qt6 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.10)

# Content of downloaded design from Figma to Qt or similar
if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/importedcontent/CMakeLists.txt")
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/importedcontent)
endif()

qt_add_executable(appygoCardSimulator
    src/app/main.cpp
)

qt_add_qml_module(appygoCardSimulator
    URI ygoCardSimulator
    QML_FILES
        qml/Main.qml
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appygoCardSimulator PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appygoCardSimulator
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(appygoCardSimulator
    PRIVATE Qt6::Quick
)

install(TARGETS appygoCardSimulator
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
