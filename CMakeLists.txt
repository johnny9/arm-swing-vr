# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.24)
project(ArmSwingVR VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

include(CTest)
include(GNUInstallDirs)
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Widgets)

option(ARMSWING_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
add_library(armswing_options INTERFACE)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(armswing_options INTERFACE -Wall -Wextra -Wpedantic)
    if(ARMSWING_ENABLE_SANITIZERS)
        target_compile_options(armswing_options INTERFACE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(armswing_options INTERFACE -fsanitize=address,undefined)
    endif()
elseif(ARMSWING_ENABLE_SANITIZERS)
    message(FATAL_ERROR "Sanitizer builds currently require GCC or Clang")
endif()

add_library(armswing_profiles STATIC src/profile.cpp)
target_include_directories(armswing_profiles PUBLIC include)
target_link_libraries(armswing_profiles PUBLIC Qt6::Core armswing_options)

add_executable(arm-swing-vr src/main.cpp src/profile_editor.cpp)
target_link_libraries(arm-swing-vr PRIVATE armswing_profiles Qt6::Widgets)
target_compile_definitions(arm-swing-vr PRIVATE ARMSWING_VERSION="${PROJECT_VERSION}")

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(profile_tests tests/profile_tests.cpp)
    target_link_libraries(profile_tests PRIVATE armswing_profiles Qt6::Test)
    add_test(NAME profiles COMMAND profile_tests)

    add_executable(editor_tests tests/editor_tests.cpp src/profile_editor.cpp)
    target_include_directories(editor_tests PRIVATE src)
    target_link_libraries(editor_tests PRIVATE armswing_profiles Qt6::Widgets Qt6::Test)
    add_test(NAME editor COMMAND editor_tests)
    set_tests_properties(editor PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

install(TARGETS arm-swing-vr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES LICENSE THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/arm-swing-vr)
install(FILES README.md docs/LICENSING.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
