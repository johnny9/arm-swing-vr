// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#define OPENVR_INTERFACE_INTERNAL
#include ARMSWING_VR_HEADER
#include "../test_support.h"
#include "fixture.h"
#include <armswing/control.h>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <thread>
namespace tables {
using namespace vr;
#include ARMSWING_VR_FORWARDERS
} // namespace tables
using namespace armswing;
int main(int argc, char** argv) {
    CHECK(argc == 5);
    setEnvironment("ARMSWING_OPENVR_REAL", argv[2]);
    setEnvironment("ARMSWING_CONTROL_FILE", argv[3]);
    Library fixture(argv[2]);
    Library adapter(argv[1]);
    const auto setFrame = fixture.get<void (*)(const VrFixtureFrame*)>("fixtureSetFrame");
    const auto get = adapter.get<void* (*)(const char*, int*)>("VR_GetGenericInterface");
    const bool flat = std::string(argv[4]) == "flat";
    int error = 0;
    tables::SystemFlatForward flatSystem;
    auto* ptr =
        get(((flat ? "FnTable:" : "") + std::string(vr::IVRSystem_Version)).c_str(), &error);
    CHECK(ptr && error == 0);
    flatSystem.next = static_cast<tables::SystemTable*>(ptr);
    auto* system =
        flat ? static_cast<vr::IVRSystem*>(&flatSystem) : static_cast<vr::IVRSystem*>(ptr);
    uint32_t w = 0, h = 0;
    system->GetRecommendedRenderTargetSize(&w, &h);
    CHECK(w == 1234 && h == 678);
    ControlPacket config;
    config.enabled = 1;
    config.generation = 1;
    config.motion.mappingCount = 1;
    config.motion.mappings[0] = {0, 3};
    VrFixtureFrame frame;
    bool actionMode = false;
    vr::VRControllerState_t left{}, right{};
    auto tick = [&] {
        config.heartbeatNs = wallTimeNs();
        CHECK(writeControl(argv[3], config));
        setFrame(&frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        if (!actionMode) {
            CHECK(system->GetControllerState(1, &left, sizeof(left)));
            CHECK(system->GetControllerState(2, &right, sizeof(right)));
        }
    };
    for (int i = 0; i < 35; ++i) {
        frame.swing = .12f * std::sin(float(i) * .2f);
        tick();
    }
    CHECK(left.rAxis[0].y > .1f && left.rAxis[0].y <= 1);
    CHECK(!(left.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A)));
    CHECK(right.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu));
    CHECK(left.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip));
    const auto movingPacket = left.unPacketNum;
    frame.manualX = .8f;
    frame.manualY = -.3f;
    tick();
    CHECK(left.rAxis[0].x == .8f && left.rAxis[0].y == -.3f);
    frame.manualX = frame.manualY = 0;
    frame.activation = false;
    tick();
    CHECK(left.rAxis[0].y == 0 &&
          !(right.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)));
    CHECK(left.unPacketNum != movingPacket);
    frame.activation = true;
    frame.focused = false;
    tick();
    CHECK(left.rAxis[0].y == 0 && (left.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A)));
    frame.focused = true;
    frame.tracked = false;
    tick();
    CHECK(left.rAxis[0].y == 0);
    frame.tracked = true;
    config.enabled = 0;
    tick();
    CHECK(left.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A));
    config.enabled = 1;
#ifndef ARMSWING_LEGACY_SYSTEM
    tables::InputFlatForward flatInput;
    ptr = get(((flat ? "FnTable:" : "") + std::string(vr::IVRInput_Version)).c_str(), &error);
    CHECK(ptr && error == 0);
    flatInput.next = static_cast<tables::InputTable*>(ptr);
    auto* input = flat ? static_cast<vr::IVRInput*>(&flatInput) : static_cast<vr::IVRInput*>(ptr);
    actionMode = true;
    frame.legacyDisabled = true;
    for (const auto* name : {"a", "b", "move"}) {
        vr::VRActionHandle_t handle = 0;
        CHECK(input->GetActionHandle(name, &handle) == vr::VRInputError_None && handle);
    }
    vr::VRActiveActionSet_t set{};
    set.ulActionSet = 1;
    vr::InputAnalogActionData_t move{};
    auto sync = [&] {
        CHECK(input->UpdateActionState(&set, sizeof(set), 1) == vr::VRInputError_None);
    };
    for (int i = 0; i < 30; ++i) {
        frame.swing = .12f * std::sin(float(i) * .2f);
        tick();
        sync();
    }
    CHECK(input->GetAnalogActionData(12, &move, sizeof(move), 1) == vr::VRInputError_None);
    CHECK(move.y > .1f);
    auto copy = move;
    CHECK(input->GetAnalogActionData(12, &move, sizeof(move), 1) == vr::VRInputError_None);
    CHECK(std::memcmp(&copy, &move, sizeof(copy)) == 0);
    vr::InputDigitalActionData_t button{};
    CHECK(input->GetDigitalActionData(10, &button, sizeof(button), 1) == vr::VRInputError_None);
    CHECK(!button.bState);
    CHECK(input->GetDigitalActionData(11, &button, sizeof(button), 2) == vr::VRInputError_None);
    CHECK(button.bState);
    copy = move;
    CHECK(input->GetAnalogActionData(12, &move, sizeof(move), 2) == vr::VRInputError_None);
    CHECK(move.y == 0);
    frame.activation = false;
    tick();
    sync();
    CHECK(input->GetAnalogActionData(12, &move, sizeof(move), 1) == vr::VRInputError_None);
    CHECK(move.y == 0 && move.deltaY < 0);
    CHECK(input->GetDigitalActionData(11, &button, sizeof(button), 2) == vr::VRInputError_None);
    CHECK(!button.bState && button.bChanged);
    tick();
    sync();
    tick();
    sync();
    CHECK(input->GetDigitalActionData(11, &button, sizeof(button), 2) == vr::VRInputError_None);
    CHECK(!button.bChanged);
    frame.failRead = true;
    setFrame(&frame);
    CHECK(input->GetDigitalActionData(11, &button, sizeof(button), 2) ==
          vr::VRInputError_InvalidParam);
    frame.failRead = false;
    frame.activation = true;
    frame.unsupportedBinding = true;
    tick();
    sync();
    CHECK(input->GetDigitalActionData(10, &button, sizeof(button), 1) == vr::VRInputError_None);
    CHECK(button.bState);
#endif
    frame.legacyDisabled = false;
    setFrame(&frame);
    config.heartbeatNs = wallTimeNs() - 1000000000;
    CHECK(writeControl(argv[3], config));
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK(system->GetControllerState(1, &left, sizeof(left)));
    CHECK(left.rAxis[0].y == 0);
    CHECK(!system->GetControllerState(1, &left, 1));
    adapter.get<void (*)()>("VR_ShutdownInternal")();
    std::filesystem::remove(argv[3]);
    std::filesystem::remove(std::string(argv[3]) + ".status");
    std::cout << vr::IVRSystem_Version << ' ' << (flat ? "flat table" : "C++")
              << " plugin integration passed\n";
}
