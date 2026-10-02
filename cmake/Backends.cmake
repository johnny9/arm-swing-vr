# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
find_package(Python3 REQUIRED COMPONENTS Interpreter)
foreach(variant IN ITEMS Current 22 19)
    if(variant STREQUAL "Current")
        set(header "${PROJECT_SOURCE_DIR}/third_party/openvr/headers/openvr.h")
    elseif(variant STREQUAL "22")
        set(header "${PROJECT_SOURCE_DIR}/third_party/openvr/legacy/openvr_2_0_10.h")
    else()
        set(header "${PROJECT_SOURCE_DIR}/third_party/openvr/legacy/openvr_1_0_17.h")
    endif()
    set(forwarders "${PROJECT_BINARY_DIR}/generated/openvr_${variant}.h")
    file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated")
    add_custom_command(OUTPUT "${forwarders}"
        COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tools/generate_openvr_forwarders.py" "${header}" "${forwarders}"
        DEPENDS "${header}" "${PROJECT_SOURCE_DIR}/tools/generate_openvr_forwarders.py" VERBATIM)
    add_library(openvr_${variant} OBJECT backends/openvr/adapter.cpp "${forwarders}")
    target_compile_definitions(openvr_${variant} PRIVATE
        ARMSWING_VR_HEADER="${header}"
        ARMSWING_VR_NAMESPACE=vr_${variant}
        ARMSWING_ADAPTER_NAMESPACE=adapter_${variant}
        ARMSWING_VR_FORWARDERS="${forwarders}"
        ARMSWING_WRAP_FUNCTION=wrapOpenVr${variant}
        ARMSWING_RESET_FUNCTION=resetOpenVr${variant})
    if(variant STREQUAL "19")
        target_compile_definitions(openvr_${variant} PRIVATE ARMSWING_LEGACY_SYSTEM)
    endif()
    target_link_libraries(openvr_${variant} PRIVATE armswing_motion)
    if(BUILD_TESTING)
        add_library(fixture_openvr_${variant} OBJECT tests/openvr/fixture_adapter.cpp "${forwarders}")
        target_compile_definitions(fixture_openvr_${variant} PRIVATE
            ARMSWING_VR_HEADER="${header}" ARMSWING_VR_NAMESPACE=vr_${variant}
            ARMSWING_ADAPTER_NAMESPACE=fixture_adapter_${variant}
            ARMSWING_VR_FORWARDERS="${forwarders}" ARMSWING_WRAP_FUNCTION=fixture${variant})
        target_link_libraries(fixture_openvr_${variant} PRIVATE armswing_options)
        add_executable(openvr_integration_${variant} tests/openvr/integration.cpp "${forwarders}")
        target_compile_definitions(openvr_integration_${variant} PRIVATE
            ARMSWING_VR_HEADER="${header}" ARMSWING_VR_FORWARDERS="${forwarders}" vr=vr_${variant})
        target_link_libraries(openvr_integration_${variant} PRIVATE armswing_motion ${CMAKE_DL_LIBS})
        if(variant STREQUAL "19")
            target_compile_definitions(fixture_openvr_${variant} PRIVATE ARMSWING_LEGACY_SYSTEM)
            target_compile_definitions(openvr_integration_${variant} PRIVATE ARMSWING_LEGACY_SYSTEM)
        endif()
        foreach(abi IN ITEMS cpp flat)
            add_test(NAME openvr_${variant}_${abi} COMMAND openvr_integration_${variant}
                $<TARGET_FILE:armswing_openvr> $<TARGET_FILE:fixture_openvr>
                "${PROJECT_BINARY_DIR}/openvr-${variant}-${abi}.control" ${abi})
            set_tests_properties(openvr_${variant}_${abi} PROPERTIES LABELS integration)
        endforeach()
    endif()
endforeach()
add_library(armswing_openvr SHARED backends/openvr/exports.cpp
    $<TARGET_OBJECTS:openvr_Current> $<TARGET_OBJECTS:openvr_22> $<TARGET_OBJECTS:openvr_19>)
target_link_libraries(armswing_openvr PRIVATE armswing_motion ${CMAKE_DL_LIBS})
if(WIN32)
    set_target_properties(armswing_openvr PROPERTIES PREFIX "" OUTPUT_NAME "openvr_api")
    target_link_options(armswing_openvr PRIVATE -static-libgcc -static-libstdc++)
endif()
install(TARGETS armswing_openvr LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/arm-swing-vr
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/arm-swing-vr)
install(FILES third_party/openvr/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/arm-swing-vr/openvr)
install(FILES third_party/openxr/LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/arm-swing-vr/openxr)

add_library(armswing_openxr SHARED backends/openxr/layer.cpp)
target_include_directories(armswing_openxr PRIVATE third_party/openxr/include)
target_link_libraries(armswing_openxr PRIVATE armswing_motion)
target_compile_options(armswing_openxr PRIVATE -Wno-missing-field-initializers)
if(WIN32)
    set_target_properties(armswing_openxr PROPERTIES PREFIX "")
    target_link_options(armswing_openxr PRIVATE -static-libgcc -static-libstdc++)
endif()
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/openxr/armswing.json" CONTENT
"{\n  \"file_format_version\": \"1.0.0\",\n  \"api_layer\": {\n    \"name\": \"XR_APILAYER_ARMSWING_locomotion\",\n    \"library_path\": \"../$<TARGET_FILE_NAME:armswing_openxr>\",\n    \"api_version\": \"1.0\",\n    \"implementation_version\": \"1\",\n    \"description\": \"Arm Swing VR per-game input layer\"\n  }\n}\n")
install(TARGETS armswing_openxr LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/arm-swing-vr
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/arm-swing-vr)
file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/install-manifest/armswing.json" CONTENT
    "{\"file_format_version\":\"1.0.0\",\"api_layer\":{\"name\":\"XR_APILAYER_ARMSWING_locomotion\",\"library_path\":\"../../../${CMAKE_INSTALL_LIBDIR}/arm-swing-vr/$<TARGET_FILE_NAME:armswing_openxr>\",\"api_version\":\"1.0\",\"implementation_version\":\"1\",\"description\":\"Arm Swing VR\"}}\n")
install(FILES "${PROJECT_BINARY_DIR}/install-manifest/armswing.json" DESTINATION ${CMAKE_INSTALL_DATADIR}/arm-swing-vr/openxr)
install(FILES LICENSE THIRD_PARTY.md DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/arm-swing-vr)
if(BUILD_TESTING)
    add_test(NAME reversible_game_setup COMMAND Python3::Interpreter "${PROJECT_SOURCE_DIR}/tests/installer_tests.py")
    add_library(fixture_openvr SHARED tests/openvr/fixture_exports.cpp
        $<TARGET_OBJECTS:fixture_openvr_Current> $<TARGET_OBJECTS:fixture_openvr_22> $<TARGET_OBJECTS:fixture_openvr_19>)
    target_link_libraries(fixture_openvr PRIVATE armswing_options)
    add_executable(motion_tests tests/motion_tests.cpp)
    target_link_libraries(motion_tests PRIVATE armswing_motion)
    add_test(NAME motion COMMAND motion_tests "${PROJECT_BINARY_DIR}/motion.control")
    add_library(fixture_openxr SHARED tests/openxr/runtime.cpp)
    target_include_directories(fixture_openxr PRIVATE third_party/openxr/include)
    target_link_libraries(fixture_openxr PRIVATE armswing_options)
    target_compile_options(fixture_openxr PRIVATE -Wno-missing-field-initializers)
    add_executable(openxr_integration tests/openxr/integration.cpp)
    target_include_directories(openxr_integration PRIVATE third_party/openxr/include)
    target_link_libraries(openxr_integration PRIVATE armswing_motion ${CMAKE_DL_LIBS})
    target_compile_options(openxr_integration PRIVATE -Wno-missing-field-initializers)
    file(GENERATE OUTPUT "${PROJECT_BINARY_DIR}/fixtures/openxr-runtime.json" CONTENT
        "{\"file_format_version\":\"1.0.0\",\"runtime\":{\"library_path\":\"../$<TARGET_FILE_NAME:fixture_openxr>\"}}\n")
    if(NOT WIN32)
        find_library(ARMSWING_OPENXR_LOADER NAMES openxr_loader REQUIRED)
        add_test(NAME openxr_loader_integration COMMAND openxr_integration
            "${ARMSWING_OPENXR_LOADER}" $<TARGET_FILE:fixture_openxr>
            "${PROJECT_BINARY_DIR}/fixtures/openxr-runtime.json" "${PROJECT_BINARY_DIR}/openxr"
            "${PROJECT_BINARY_DIR}/openxr.control" $<TARGET_FILE:armswing_openxr>)
        set_tests_properties(openxr_loader_integration PROPERTIES LABELS integration)
    endif()
endif()
add_executable(openvr_probe tools/openvr_probe.cpp)
target_include_directories(openvr_probe PRIVATE third_party/openvr/headers)
target_link_libraries(openvr_probe PRIVATE armswing_openvr armswing_options ${CMAKE_DL_LIBS})
add_executable(openxr_probe tools/openxr_probe.cpp)
target_include_directories(openxr_probe PRIVATE third_party/openxr/include)
target_link_libraries(openxr_probe PRIVATE armswing_options ${CMAKE_DL_LIBS})
target_compile_options(openxr_probe PRIVATE -Wno-missing-field-initializers)
