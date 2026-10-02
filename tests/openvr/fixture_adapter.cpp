// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#define OPENVR_INTERFACE_INTERNAL
#define vr ARMSWING_VR_NAMESPACE
#include ARMSWING_VR_HEADER
#include "fixture.h"
#include <cstring>
#include <string>
namespace ARMSWING_ADAPTER_NAMESPACE {
using namespace vr;
#include ARMSWING_VR_FORWARDERS
struct System : SystemDefaults {
    void GetRecommendedRenderTargetSize(uint32_t* w, uint32_t* h) override {
        *w = 1234;
        *h = 678;
    }
    bool IsInputAvailable() override { return vrFixture.focused; }
    vr::TrackedDeviceIndex_t
    GetTrackedDeviceIndexForControllerRole(vr::ETrackedControllerRole r) override {
        return r == vr::TrackedControllerRole_LeftHand ? 1 : 2;
    }
    vr::ETrackedControllerRole
    GetControllerRoleForTrackedDeviceIndex(vr::TrackedDeviceIndex_t i) override {
        return i == 1 ? vr::TrackedControllerRole_LeftHand : vr::TrackedControllerRole_RightHand;
    }
    void GetDeviceToAbsoluteTrackingPose(vr::ETrackingUniverseOrigin, float,
                                         vr::TrackedDevicePose_t* poses, uint32_t count) override {
        for (unsigned i = 0; i < count; ++i) {
            poses[i] = {};
            if (i > 2)
                continue;
            poses[i].bDeviceIsConnected = true;
            poses[i].bPoseIsValid = vrFixture.tracked;
            poses[i].eTrackingResult = vr::TrackingResult_Running_OK;
            auto& m = poses[i].mDeviceToAbsoluteTracking.m;
            m[0][0] = m[1][1] = m[2][2] = 1;
            m[0][3] = i == 1 ? -.3f : i == 2 ? .3f : 0;
            m[1][3] = i == 0 ? 1.7f : 1 + (i == 1 ? vrFixture.swing : -vrFixture.swing);
        }
    }
    bool GetControllerState(vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state,
                            uint32_t size) override {
        if (!state || size != sizeof(*state) || vrFixture.failRead || vrFixture.legacyDisabled)
            return false;
        *state = {};
        state->unPacketNum = 42;
        state->ulButtonPressed = vr::ButtonMaskFromId(vr::k_EButton_Grip);
        if (index == 1 && vrFixture.activation)
            state->ulButtonPressed |= vr::ButtonMaskFromId(vr::k_EButton_A);
        if (index == 1)
            state->rAxis[0] = {vrFixture.manualX, vrFixture.manualY};
        return true;
    }
    bool GetControllerStateWithPose(vr::ETrackingUniverseOrigin, vr::TrackedDeviceIndex_t i,
                                    vr::VRControllerState_t* s, uint32_t n,
                                    vr::TrackedDevicePose_t* p) override {
        if (p) {
            *p = {};
            p->bPoseIsValid = true;
        }
        return GetControllerState(i, s, n);
    }
    int32_t GetInt32TrackedDeviceProperty(vr::TrackedDeviceIndex_t, vr::ETrackedDeviceProperty p,
                                          vr::ETrackedPropertyError* e) override {
        if (e)
            *e = vr::TrackedProp_Success;
        return p == vr::Prop_Axis0Type_Int32 ? vr::k_eControllerAxis_Joystick
                                             : vr::k_eControllerAxis_None;
    }
};
static System fixtureSystem;
#ifndef ARMSWING_LEGACY_SYSTEM
struct Input : InputDefaults {
    vr::EVRInputError GetActionHandle(const char* name, vr::VRActionHandle_t* out) override {
        *out = std::strcmp(name, "a") == 0 ? 10 : std::strcmp(name, "b") == 0 ? 11 : 12;
        return vr::VRInputError_None;
    }
    vr::EVRInputError UpdateActionState(vr::VRActiveActionSet_t*, uint32_t, uint32_t) override {
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetInputSourceHandle(const char* name,
                                           vr::VRInputValueHandle_t* value) override {
        *value = std::strstr(name, "right") ? 2 : 1;
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetDigitalActionData(vr::VRActionHandle_t a, vr::InputDigitalActionData_t* d,
                                           uint32_t n, vr::VRInputValueHandle_t) override {
        if (n != sizeof(*d) || vrFixture.failRead)
            return vr::VRInputError_InvalidParam;
        if (a >= 12)
            return vr::VRInputError_WrongType;
        *d = {};
        d->bActive = vrFixture.actionActive;
        d->bState = a == 10 && vrFixture.activation;
        d->activeOrigin = 1;
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetAnalogActionData(vr::VRActionHandle_t, vr::InputAnalogActionData_t* d,
                                          uint32_t n, vr::VRInputValueHandle_t) override {
        if (n != sizeof(*d) || vrFixture.failRead)
            return vr::VRInputError_InvalidParam;
        *d = {};
        d->bActive = vrFixture.actionActive;
        d->x = vrFixture.manualX;
        d->y = vrFixture.manualY;
        d->activeOrigin = 1;
        return vr::VRInputError_None;
    }
    vr::EVRInputError GetActionBindingInfo(vr::VRActionHandle_t a, vr::InputBindingInfo_t* d,
                                           uint32_t, uint32_t, uint32_t* count) override {
        *count = 1;
        *d = {};
        std::strcpy(d->rchDevicePathName, a == 11 ? "/user/hand/right" : "/user/hand/left");
        std::strcpy(d->rchInputPathName, a == 10   ? "/input/a"
                                         : a == 11 ? "/input/b"
                                                   : "/input/thumbstick");
        std::strcpy(d->rchModeName, vrFixture.unsupportedBinding ? "toggle"
                                    : a < 12                     ? "button"
                                                                 : "joystick");
        std::strcpy(d->rchSlotName, a < 12 ? "click" : "position");
        return vr::VRInputError_None;
    }
};
static Input input;
#endif
} // namespace ARMSWING_ADAPTER_NAMESPACE
void* ARMSWING_WRAP_FUNCTION(const char* version) {
    using namespace ARMSWING_ADAPTER_NAMESPACE;
    if (std::strcmp(version, vr::IVRSystem_Version) == 0)
        return &fixtureSystem;
    if (std::string(version) == std::string("FnTable:") + vr::IVRSystem_Version) {
        SystemTrampoline::target = &fixtureSystem;
        return &SystemTrampoline::table;
    }
#ifndef ARMSWING_LEGACY_SYSTEM
    if (std::strcmp(version, vr::IVRInput_Version) == 0)
        return &input;
    if (std::string(version) == std::string("FnTable:") + vr::IVRInput_Version) {
        InputTrampoline::target = &input;
        return &InputTrampoline::table;
    }
#endif
    return nullptr;
}
