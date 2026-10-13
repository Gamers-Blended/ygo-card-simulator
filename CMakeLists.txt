cmake_minimum_required(VERSION 3.16)

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

project(ygoCardSimulator VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20) # Compile with C++20
set(CMAKE_CXX_STANDARD_REQUIRED ON) # If compiler can't do C++20, CMake stops with an error instead of quietly using older version
set(CMAKE_CXX_EXTENSIONS OFF) # Strict standard C++20 (-std=c++20, not gnu++20) so compiler-specific extensions don't sneak in and break MSVC builds

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
