// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// Compiled separately for each pinned interface ABI, in separate namespaces.
#define OPENVR_INTERFACE_INTERNAL
#define vr ARMSWING_VR_NAMESPACE
#include ARMSWING_VR_HEADER
#include <algorithm>
#include <armswing/control.h>
#include <cmath>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace ARMSWING_ADAPTER_NAMESPACE {
using namespace vr;
using namespace armswing;
#include ARMSWING_VR_FORWARDERS
using Factory = void* (*)(const char*, int*);
struct Context {
    std::recursive_mutex mutex;
    Factory factory = nullptr;
    vr::IVRSystem* system = nullptr;
    ControlPacket control{};
    MotionProcessor processor;
    Output output;
    int64_t sampled = 0;
    uint64_t frame = 0;
    std::array<vr::VRControllerState_t, 2> delivered{};
    std::array<bool, 2> haveDelivered{};
    void reset() {
        processor.reset();
        output = {};
        sampled = 0;
        ++frame;
    }
    void capture(bool force = false, const std::array<bool, 4>* buttons = nullptr,
                 unsigned knownButtons = 0) {
        const auto now = steadyTimeNs();
        if (!force && now - sampled < 2000000)
            return;
        sampled = now;
        ++frame;
        ControlPacket candidate;
        if (!readControl(Backend::OpenVr, candidate)) {
            processor.reset();
            output = {};
            reportStatus(Backend::OpenVr, output);
            return;
        }
        if (candidate.generation != control.generation)
            processor.reset();
        control = candidate;
        if (!system && factory) {
            int error = 0;
            system = static_cast<vr::IVRSystem*>(factory(vr::IVRSystem_Version, &error));
        }
        Sample s;
        s.timeNs = now;
        if (system) {
            s.focused = system->IsInputAvailable();
            vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};
            system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, poses,
                                                    vr::k_unMaxTrackedDeviceCount);
            auto pose = [&](unsigned index) {
                Pose p;
                if (index >= vr::k_unMaxTrackedDeviceCount)
                    return p;
                const auto& raw = poses[index];
                p.valid = raw.bDeviceIsConnected && raw.bPoseIsValid &&
                          raw.eTrackingResult == vr::TrackingResult_Running_OK;
                p.position = {raw.mDeviceToAbsoluteTracking.m[0][3],
                              raw.mDeviceToAbsoluteTracking.m[1][3],
                              raw.mDeviceToAbsoluteTracking.m[2][3]};
                p.forward = {-raw.mDeviceToAbsoluteTracking.m[0][2],
                             -raw.mDeviceToAbsoluteTracking.m[2][2]};
                return p;
            };
            s.head = pose(vr::k_unTrackedDeviceIndex_Hmd);
            for (unsigned hand = 0; hand < 2; ++hand) {
                const auto index = system->GetTrackedDeviceIndexForControllerRole(
                    hand ? vr::TrackedControllerRole_RightHand
                         : vr::TrackedControllerRole_LeftHand);
                s.hands[hand] = pose(index);
                vr::VRControllerState_t state{};
                if (buttons) {
                    s.buttons[hand * 2] = (*buttons)[hand * 2];
                    s.buttons[hand * 2 + 1] = (*buttons)[hand * 2 + 1];
                } else if (index < vr::k_unMaxTrackedDeviceCount &&
                           system->GetControllerState(index, &state, sizeof(state))) {
                    s.buttons[hand * 2] =
                        state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_A);
                    s.buttons[hand * 2 + 1] =
                        state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu);
                } else
                    s.hands[hand].valid = false;
            }
            if (buttons) {
                unsigned required = 1u << control.motion.activation;
                for (unsigned n = 0; n < control.motion.mappingCount; ++n)
                    required |= 1u << control.motion.mappings[n].source;
                s.focused = s.focused && (knownButtons & required) == required;
            }
        }
        output = processor.update(control.motion, s, true);
        reportStatus(Backend::OpenVr, output);
    }
    void controller(unsigned index, vr::VRControllerState_t& state) {
        capture();
        if (!system)
            return;
        const auto role = system->GetControllerRoleForTrackedDeviceIndex(index);
        if (role != vr::TrackedControllerRole_LeftHand &&
            role != vr::TrackedControllerRole_RightHand)
            return;
        const unsigned hand = role == vr::TrackedControllerRole_RightHand;
        if (!output.ownsInput && !haveDelivered[hand])
            return;
        if (output.ownsInput) {
            const vr::EVRButtonId ids[] = {vr::k_EButton_A, vr::k_EButton_ApplicationMenu};
            for (unsigned b = 0; b < 2; ++b) {
                const auto mask = vr::ButtonMaskFromId(ids[b]);
                state.ulButtonPressed =
                    (state.ulButtonPressed & ~mask) | (output.buttons[hand * 2 + b] ? mask : 0);
            }
            if (hand == control.motion.outputHand) {
                for (unsigned axis = 0; axis < vr::k_unControllerStateAxisCount; ++axis) {
                    vr::ETrackedPropertyError error;
                    if (system->GetInt32TrackedDeviceProperty(
                            index,
                            static_cast<vr::ETrackedDeviceProperty>(vr::Prop_Axis0Type_Int32 +
                                                                    axis),
                            &error) == vr::k_eControllerAxis_Joystick &&
                        error == vr::TrackedProp_Success) {
                        const auto value =
                            applyMovement({state.rAxis[axis].x, state.rAxis[axis].y}, output);
                        state.rAxis[axis].x = value.x;
                        state.rAxis[axis].y = value.y;
                        break;
                    }
                }
            }
        }
        const auto& previous = delivered[hand];
        const bool changed = !haveDelivered[hand] ||
                             previous.ulButtonPressed != state.ulButtonPressed ||
                             previous.ulButtonTouched != state.ulButtonTouched ||
                             std::memcmp(previous.rAxis, state.rAxis, sizeof(state.rAxis)) != 0;
        state.unPacketNum =
            haveDelivered[hand] ? previous.unPacketNum + uint32_t(changed) : state.unPacketNum + 1;
        delivered[hand] = state;
        haveDelivered[hand] = true;
    }
};
static Context context;
struct SystemProxy : SystemForward {
    bool GetControllerState(vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state,
                            uint32_t size) override {
        std::lock_guard lock(context.mutex);
        const bool result = next->GetControllerState(index, state, size);
        if (result && state && size == sizeof(*state))
            context.controller(index, *state);
        return result;
    }
    bool GetControllerStateWithPose(vr::ETrackingUniverseOrigin origin,
                                    vr::TrackedDeviceIndex_t index, vr::VRControllerState_t* state,
                                    uint32_t size, vr::TrackedDevicePose_t* pose) override {
        std::lock_guard lock(context.mutex);
        const bool result = next->GetControllerStateWithPose(origin, index, state, size, pose);
        if (result && state && size == sizeof(*state))
            context.controller(index, *state);
        return result;
    }
};
#ifndef ARMSWING_LEGACY_SYSTEM
struct InputProxy : InputForward {
    using Key = std::pair<uint64_t, uint64_t>;
    template <class T> struct Cached {
        uint64_t frame = 0;
        T data{};
        bool exists = false;
    };
    std::map<Key, Cached<vr::InputDigitalActionData_t>> digital;
    std::map<Key, Cached<vr::InputAnalogActionData_t>> analog;
    std::set<vr::VRActionHandle_t> actions;
    uint64_t frameNumber = 0;
    Output actionOutput;
    ControlPacket actionControl;
    vr::EVRInputError GetActionHandle(const char* name, vr::VRActionHandle_t* handle) override {
        std::lock_guard lock(context.mutex);
        const auto result = next->GetActionHandle(name, handle);
        if (result == vr::VRInputError_None && handle)
            actions.insert(*handle);
        return result;
    }
    // Only simple, unambiguous boolean and joystick bindings are rewritten.
    std::vector<std::string> paths(vr::VRActionHandle_t action, vr::VRInputValueHandle_t restrict) {
        vr::InputBindingInfo_t info[32]{};
        uint32_t count = 0;
        if (next->GetActionBindingInfo(action, info, sizeof(info[0]), 32, &count) !=
                vr::VRInputError_None ||
            count > 32)
            return {};
        std::vector<std::string> result;
        for (unsigned i = 0; i < count; ++i) {
            const std::string hand(info[i].rchDevicePathName,
                                   strnlen(info[i].rchDevicePathName, 128));
            if (restrict) {
                vr::VRInputValueHandle_t handle = 0;
                if (next->GetInputSourceHandle(hand.c_str(), &handle) != vr::VRInputError_None ||
                    handle != restrict)
                    continue;
            }
            std::string path(info[i].rchInputPathName, strnlen(info[i].rchInputPathName, 128));
            const std::string mode(info[i].rchModeName, strnlen(info[i].rchModeName, 128));
            const std::string slot(info[i].rchSlotName, strnlen(info[i].rchSlotName, 128));
            if (!path.starts_with("/user/"))
                path = hand + path;
            if (mode == "button" && slot == "click") {
                if (!path.ends_with("/click"))
                    path += "/click";
            } else if (mode != "joystick" || slot != "position")
                path = "unsupported";
            result.push_back(path);
        }
        return result;
    }
    vr::EVRInputError UpdateActionState(vr::VRActiveActionSet_t* sets, uint32_t size,
                                        uint32_t count) override {
        std::lock_guard lock(context.mutex);
        const auto error = next->UpdateActionState(sets, size, count);
        if (error == vr::VRInputError_None && count) {
            // Legacy GetControllerState is unavailable in action-based games.
            // Recover only unambiguous, currently active physical click bindings.
            std::array<bool, 4> buttons{};
            unsigned known = 0;
            for (const auto action : actions) {
                for (unsigned hand = 0; hand < 2; ++hand) {
                    vr::VRInputValueHandle_t device = 0;
                    if (next->GetInputSourceHandle(hand ? "/user/hand/right" : "/user/hand/left",
                                                   &device) != vr::VRInputError_None)
                        continue;
                    vr::InputDigitalActionData_t data{};
                    if (next->GetDigitalActionData(action, &data, sizeof(data), device) !=
                            vr::VRInputError_None ||
                        !data.bActive)
                        continue;
                    const auto bound = paths(action, device);
                    if (bound.size() != 1)
                        continue;
                    const int index = buttonIndex(bound.front());
                    if (index < 0)
                        continue;
                    known |= 1u << index;
                    buttons[index] = buttons[index] || data.bState;
                }
            }
            context.capture(true, &buttons, known);
        } else
            context.reset();
        ++frameNumber;
        actionOutput = context.output;
        actionControl = context.control;
        // Sample previously requested actions every sync, even when the game
        // skips querying them in a frame, to preserve change/delta semantics.
        for (auto& [key, cached] : digital) {
            vr::InputDigitalActionData_t data{};
            GetDigitalActionData(key.first, &data, sizeof(data), key.second);
        }
        for (auto& [key, cached] : analog) {
            vr::InputAnalogActionData_t data{};
            GetAnalogActionData(key.first, &data, sizeof(data), key.second);
        }
        return error;
    }
    vr::EVRInputError GetDigitalActionData(vr::VRActionHandle_t action,
                                           vr::InputDigitalActionData_t* data, uint32_t size,
                                           vr::VRInputValueHandle_t restrict) override {
        std::lock_guard lock(context.mutex);
        const auto error = next->GetDigitalActionData(action, data, size, restrict);
        if (error != vr::VRInputError_None || !data || size != sizeof(*data))
            return error;
        auto& cache = digital[{action, restrict}];
        if (cache.exists && cache.frame == frameNumber) {
            *data = cache.data;
            return error;
        }
        const bool physical = data->bState;
        if (actionOutput.ownsInput && data->bActive) {
            const auto bound = paths(action, restrict);
            bool supported = !bound.empty(), pressed = false;
            for (const auto& path : bound) {
                const int button = buttonIndex(path);
                if (button < 0) {
                    supported = false;
                    break;
                }
                pressed |= actionOutput.buttons[button];
            }
            if (supported)
                data->bState = pressed;
        }
        if (data->bActive &&
            (actionOutput.ownsInput || (cache.exists && cache.data.bState != physical))) {
            data->bChanged = cache.exists ? data->bState != cache.data.bState
                                          : data->bState != physical || data->bChanged;
            if (data->bChanged)
                data->fUpdateTime = 0;
        }
        cache = {frameNumber, *data, true};
        return error;
    }
    vr::EVRInputError GetAnalogActionData(vr::VRActionHandle_t action,
                                          vr::InputAnalogActionData_t* data, uint32_t size,
                                          vr::VRInputValueHandle_t restrict) override {
        std::lock_guard lock(context.mutex);
        const auto error = next->GetAnalogActionData(action, data, size, restrict);
        if (error != vr::VRInputError_None || !data || size != sizeof(*data))
            return error;
        auto& cache = analog[{action, restrict}];
        if (cache.exists && cache.frame == frameNumber) {
            *data = cache.data;
            return error;
        }
        const auto before = *data;
        if (actionOutput.ownsInput && data->bActive) {
            const auto bound = paths(action, restrict);
            const std::string hand = actionControl.motion.outputHand ? "right" : "left";
            if (!bound.empty() && std::all_of(bound.begin(), bound.end(), [&](const auto& path) {
                    return path == "/user/hand/" + hand + "/input/thumbstick" ||
                           path == "/user/hand/" + hand + "/input/joystick";
                })) {
                const auto value = applyMovement({data->x, data->y}, actionOutput);
                data->x = value.x;
                data->y = value.y;
            }
        }
        if (data->bActive &&
            (actionOutput.ownsInput ||
             (cache.exists && (cache.data.x != before.x || cache.data.y != before.y)))) {
            data->deltaX = data->x - (cache.exists ? cache.data.x : before.x - before.deltaX);
            data->deltaY = data->y - (cache.exists ? cache.data.y : before.y - before.deltaY);
            if (data->deltaX || data->deltaY)
                data->fUpdateTime = 0;
        }
        cache = {frameNumber, *data, true};
        return error;
    }
};
#endif
static SystemProxy systemCpp, systemFlat;
static SystemFlatForward realSystemFlat;
#ifndef ARMSWING_LEGACY_SYSTEM
static InputProxy inputCpp, inputFlat;
static InputFlatForward realInputFlat;
#endif
} // namespace ARMSWING_ADAPTER_NAMESPACE

void* ARMSWING_WRAP_FUNCTION(const char* version, void* original,
                             void* (*factory)(const char*, int*)) {
    using namespace ARMSWING_ADAPTER_NAMESPACE;
    std::lock_guard lock(context.mutex);
    context.factory = factory;
    if (!original)
        return nullptr;
    const std::string requested(version);
    if (requested == vr::IVRSystem_Version) {
        systemCpp.next = static_cast<vr::IVRSystem*>(original);
        context.system = systemCpp.next;
        return &systemCpp;
    }
    if (requested == std::string("FnTable:") + vr::IVRSystem_Version) {
        realSystemFlat.next = static_cast<SystemTable*>(original);
        systemFlat.next = &realSystemFlat;
        context.system = &realSystemFlat;
        SystemTrampoline::target = &systemFlat;
        return &SystemTrampoline::table;
    }
#ifndef ARMSWING_LEGACY_SYSTEM
    if (requested == vr::IVRInput_Version) {
        inputCpp.next = static_cast<vr::IVRInput*>(original);
        return &inputCpp;
    }
    if (requested == std::string("FnTable:") + vr::IVRInput_Version) {
        realInputFlat.next = static_cast<InputTable*>(original);
        inputFlat.next = &realInputFlat;
        InputTrampoline::target = &inputFlat;
        return &InputTrampoline::table;
    }
#endif
    return original;
}
void ARMSWING_RESET_FUNCTION() {
    using namespace ARMSWING_ADAPTER_NAMESPACE;
    std::lock_guard lock(context.mutex);
    context.reset();
    context.system = nullptr;
    context.haveDelivered = {};
#ifndef ARMSWING_LEGACY_SYSTEM
    inputCpp.digital.clear();
    inputCpp.analog.clear();
    inputFlat.digital.clear();
    inputFlat.analog.clear();
    inputCpp.actions.clear();
    inputFlat.actions.clear();
    inputCpp.actionOutput = {};
    inputFlat.actionOutput = {};
#endif
}
