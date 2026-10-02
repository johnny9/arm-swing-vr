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
find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

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

add_library(armswing_controller STATIC src/profile_controller.cpp src/profile_controller.h)
target_include_directories(armswing_controller PUBLIC src)
target_link_libraries(armswing_controller PUBLIC armswing_profiles)

set(ARMSWING_QML
    qml/qmldir qml/Theme.qml qml/Glyph.qml qml/AppButton.qml qml/AppField.qml
    qml/AppCombo.qml qml/InputPicker.qml qml/SettingRow.qml qml/Divider.qml
    qml/MovementPage.qml qml/MappingsPage.qml qml/GamePage.qml qml/Main.qml)
add_executable(arm-swing-vr src/main.cpp)
target_link_libraries(arm-swing-vr PRIVATE armswing_controller Qt6::Quick Qt6::QuickControls2)
qt_add_resources(arm-swing-vr ui PREFIX "/" FILES ${ARMSWING_QML})
target_compile_definitions(arm-swing-vr PRIVATE ARMSWING_VERSION="${PROJECT_VERSION}")

if(BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_executable(profile_tests tests/profile_tests.cpp)
    target_link_libraries(profile_tests PRIVATE armswing_profiles Qt6::Test)
    add_test(NAME profiles COMMAND profile_tests)

    add_executable(controller_tests tests/controller_tests.cpp)
    target_link_libraries(controller_tests PRIVATE armswing_controller Qt6::Test)
    add_test(NAME controller COMMAND controller_tests)

    add_executable(qml_ui_tests tests/qml_ui_tests.cpp)
    target_link_libraries(qml_ui_tests PRIVATE armswing_controller Qt6::Quick Qt6::QuickControls2 Qt6::Test)
    qt_add_resources(qml_ui_tests ui PREFIX "/" FILES ${ARMSWING_QML})
    add_test(NAME qml_ui COMMAND qml_ui_tests)
    set_tests_properties(qml_ui PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software")
endif()

install(TARGETS arm-swing-vr RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES LICENSE THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/arm-swing-vr)
install(FILES third_party/apps-sdk-ui/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/arm-swing-vr/apps-sdk-ui)
install(FILES README.md docs/LICENSING.md DESTINATION ${CMAKE_INSTALL_DOCDIR})
