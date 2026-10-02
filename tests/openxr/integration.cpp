// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../test_support.h"
#include "fixture.h"
#include <armswing/control.h>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <vector>
using namespace armswing;
#define GET(name)                                                                                  \
    PFN_##name name = nullptr;                                                                     \
    CHECK(get(instance, #name, reinterpret_cast<PFN_xrVoidFunction*>(&name)) == XR_SUCCESS && name)
int main(int argc, char** argv) {
    CHECK(argc == 7);
    setEnvironment("XR_RUNTIME_JSON", argv[3]);
    setEnvironment("XR_API_LAYER_PATH", argv[4]);
    setEnvironment("XR_ENABLE_API_LAYERS", "XR_APILAYER_ARMSWING_locomotion");
    setEnvironment("ARMSWING_CONTROL_FILE", argv[5]);
    Library fixture(argv[2]), loader(argv[1]), layer(argv[6]);
    const auto setFrame = fixture.get<void (*)(const XrFixtureFrame*)>("fixtureSetFrame");
    const auto statsFn = fixture.get<void (*)(XrFixtureStats*)>("fixtureGetStats");
    const auto negotiate =
        layer.get<PFN_xrNegotiateLoaderApiLayerInterface>("xrNegotiateLoaderApiLayerInterface");
    CHECK(negotiate(nullptr, "wrong", nullptr) == XR_ERROR_INITIALIZATION_FAILED);
    const auto get = loader.get<PFN_xrGetInstanceProcAddr>("xrGetInstanceProcAddr");
    const auto create = loader.get<PFN_xrCreateInstance>("xrCreateInstance");
    XrInstance instance = XR_NULL_HANDLE;
    XrInstanceCreateInfo ci{};
    ci.type = XR_TYPE_INSTANCE_CREATE_INFO;
    std::strcpy(ci.applicationInfo.applicationName, "arm-swing-loader-integration");
    ci.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
    const char* headless = "XR_MND_headless";
    ci.enabledExtensionCount = 1;
    ci.enabledExtensionNames = &headless;
    CHECK(create(&ci, &instance) == XR_SUCCESS);
    GET(xrGetInstanceProperties);
    GET(xrGetSystem);
    GET(xrCreateActionSet);
    GET(xrCreateAction);
    GET(xrStringToPath);
    GET(xrSuggestInteractionProfileBindings);
    GET(xrCreateSession);
    GET(xrAttachSessionActionSets);
    GET(xrSyncActions);
    GET(xrWaitFrame);
    GET(xrBeginFrame);
    GET(xrEndFrame);
    GET(xrBeginSession);
    GET(xrEndSession);
    GET(xrGetActionStateBoolean);
    GET(xrGetActionStateFloat);
    GET(xrGetActionStateVector2f);
    GET(xrDestroySession);
    GET(xrDestroyInstance);
    XrInstanceProperties properties{};
    properties.type = XR_TYPE_INSTANCE_PROPERTIES;
    CHECK(xrGetInstanceProperties(instance, &properties) == XR_SUCCESS);
    CHECK(std::string(properties.runtimeName) == "Arm Swing integration fixture");
    XrActionSetCreateInfo setInfo{};
    setInfo.type = XR_TYPE_ACTION_SET_CREATE_INFO;
    std::strcpy(setInfo.actionSetName, "game");
    std::strcpy(setInfo.localizedActionSetName, "Game");
    XrActionSet set;
    CHECK(xrCreateActionSet(instance, &setInfo, &set) == XR_SUCCESS);
    auto path = [&](const char* text) {
        XrPath p;
        CHECK(xrStringToPath(instance, text, &p) == XR_SUCCESS);
        return p;
    };
    const XrPath hands[] = {path("/user/hand/left"), path("/user/hand/right")};
    auto action = [&](const char* name, XrActionType type) {
        XrActionCreateInfo info{};
        info.type = XR_TYPE_ACTION_CREATE_INFO;
        std::strcpy(info.actionName, name);
        std::strcpy(info.localizedActionName, name);
        info.actionType = type;
        info.countSubactionPaths = 2;
        info.subactionPaths = hands;
        XrAction a;
        CHECK(xrCreateAction(set, &info, &a) == XR_SUCCESS);
        return a;
    };
    const auto move = action("move", XR_ACTION_TYPE_VECTOR2F_INPUT),
               x = action("move_x", XR_ACTION_TYPE_FLOAT_INPUT),
               y = action("move_y", XR_ACTION_TYPE_FLOAT_INPUT),
               activation = action("original_a", XR_ACTION_TYPE_BOOLEAN_INPUT),
               destination = action("mapped_b", XR_ACTION_TYPE_BOOLEAN_INPUT),
               untouched = action("trigger", XR_ACTION_TYPE_BOOLEAN_INPUT);
    std::vector<XrActionSuggestedBinding> bindings{
        {move, path("/user/hand/left/input/thumbstick")},
        {x, path("/user/hand/left/input/thumbstick/x")},
        {y, path("/user/hand/left/input/thumbstick/y")},
        {activation, path("/user/hand/left/input/a/click")},
        {destination, path("/user/hand/right/input/b/click")},
        {untouched, path("/user/hand/left/input/trigger/click")}};
    XrInteractionProfileSuggestedBinding suggestion{};
    suggestion.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
    suggestion.interactionProfile = path("/interaction_profiles/valve/index_controller");
    suggestion.countSuggestedBindings = uint32_t(bindings.size());
    suggestion.suggestedBindings = bindings.data();
    CHECK(xrSuggestInteractionProfileBindings(instance, &suggestion) == XR_SUCCESS);
    XrSystemGetInfo systemInfo{};
    systemInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system;
    CHECK(xrGetSystem(instance, &systemInfo, &system) == XR_SUCCESS);
    XrSessionCreateInfo sessionInfo{};
    sessionInfo.type = XR_TYPE_SESSION_CREATE_INFO;
    sessionInfo.systemId = system;
    XrSession session;
    CHECK(xrCreateSession(instance, &sessionInfo, &session) == XR_SUCCESS);
    XrSessionActionSetsAttachInfo attach{};
    attach.type = XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO;
    attach.countActionSets = 1;
    attach.actionSets = &set;
    CHECK(xrAttachSessionActionSets(session, &attach) == XR_SUCCESS);
    XrSessionBeginInfo begin{};
    begin.type = XR_TYPE_SESSION_BEGIN_INFO;
    begin.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
    CHECK(xrBeginSession(session, &begin) == XR_SUCCESS);
    ControlPacket control;
    control.enabled = 1;
    control.backend = Backend::OpenXr;
    control.generation = 1;
    control.motion.mappingCount = 1;
    control.motion.mappings[0] = {0, 3};
    XrFixtureFrame frame;
    XrActiveActionSet active{set, XR_NULL_PATH};
    XrActionsSyncInfo syncInfo{};
    syncInfo.type = XR_TYPE_ACTIONS_SYNC_INFO;
    syncInfo.countActiveActionSets = 1;
    syncInfo.activeActionSets = &active;
    XrFrameState frameState{};
    frameState.type = XR_TYPE_FRAME_STATE;
    auto tick = [&] {
        control.heartbeatNs = wallTimeNs();
        CHECK(writeControl(argv[5], control));
        setFrame(&frame);
        XrFrameWaitInfo wait{};
        wait.type = XR_TYPE_FRAME_WAIT_INFO;
        CHECK(xrWaitFrame(session, &wait, &frameState) == XR_SUCCESS);
        XrFrameBeginInfo start{};
        start.type = XR_TYPE_FRAME_BEGIN_INFO;
        CHECK(xrBeginFrame(session, &start) == XR_SUCCESS);
        const auto result = xrSyncActions(session, &syncInfo);
        CHECK(result == (frame.focused ? XR_SUCCESS : XR_SESSION_NOT_FOCUSED));
        XrFrameEndInfo end{};
        end.type = XR_TYPE_FRAME_END_INFO;
        end.displayTime = frameState.predictedDisplayTime;
        end.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
        CHECK(xrEndFrame(session, &end) == XR_SUCCESS);
    };
    auto vector = [&](XrPath hand = XR_NULL_PATH) {
        XrActionStateGetInfo info{};
        info.type = XR_TYPE_ACTION_STATE_GET_INFO;
        info.action = move;
        info.subactionPath = hand;
        XrActionStateVector2f state{};
        state.type = XR_TYPE_ACTION_STATE_VECTOR2F;
        CHECK(xrGetActionStateVector2f(session, &info, &state) == XR_SUCCESS);
        return state;
    };
    auto scalar = [&](XrAction a) {
        XrActionStateGetInfo info{};
        info.type = XR_TYPE_ACTION_STATE_GET_INFO;
        info.action = a;
        info.subactionPath = hands[0];
        XrActionStateFloat state{};
        state.type = XR_TYPE_ACTION_STATE_FLOAT;
        CHECK(xrGetActionStateFloat(session, &info, &state) == XR_SUCCESS);
        return state;
    };
    auto boolean = [&](XrAction a) {
        XrActionStateGetInfo info{};
        info.type = XR_TYPE_ACTION_STATE_GET_INFO;
        info.action = a;
        XrActionStateBoolean state{};
        state.type = XR_TYPE_ACTION_STATE_BOOLEAN;
        CHECK(xrGetActionStateBoolean(session, &info, &state) == XR_SUCCESS);
        return state;
    };
    for (int i = 0; i < 60; ++i) {
        frame.swing = .15f * std::sin(float(i) * .2f);
        tick();
        vector();
        scalar(x);
        scalar(y);
        boolean(destination);
    }
    const auto v = vector(hands[0]);
    CHECK(v.currentState.y > .1f && v.currentState.y <= 1);
    CHECK(std::abs(scalar(y).currentState - v.currentState.y) < .0001f);
    CHECK(scalar(x).currentState == 0);
    const auto again = vector(hands[0]);
    CHECK(again.currentState.y == v.currentState.y && again.lastChangeTime == v.lastChangeTime &&
          again.changedSinceLastSync == v.changedSinceLastSync);
    CHECK(vector(hands[1]).currentState.y == 0 && !vector(hands[1]).isActive);
    CHECK(!boolean(activation).currentState && boolean(destination).currentState &&
          boolean(untouched).currentState);
    XrFixtureStats stats;
    statsFn(&stats);
    CHECK(stats.attachedSets == 2 && stats.syncedSets == 2 &&
          stats.suggestions == bindings.size() + 8);
    frame.manualX = .8f;
    frame.manualY = -.3f;
    tick();
    CHECK(vector().currentState.x == .8f && scalar(y).currentState == -.3f);
    frame.manualX = frame.manualY = 0;
    frame.activation = false;
    tick();
    CHECK(vector().currentState.y == 0 && !boolean(destination).currentState &&
          boolean(destination).changedSinceLastSync);
    tick();
    tick();
    CHECK(!boolean(destination).changedSinceLastSync);
    frame.activation = true;
    frame.tracked = false;
    tick();
    CHECK(vector().currentState.y == 0 && boolean(activation).currentState);
    frame.tracked = true;
    frame.focused = false;
    tick();
    CHECK(!vector().isActive);
    frame.focused = true;
    control.enabled = 0;
    tick();
    CHECK(boolean(activation).currentState && !boolean(destination).currentState);
    statsFn(&stats);
    CHECK(stats.syncedSets == 1);
    control.enabled = 1;
    control.heartbeatNs = wallTimeNs() - 1000000000;
    CHECK(writeControl(argv[5], control));
    CHECK(xrSyncActions(session, &syncInfo) == XR_SUCCESS);
    CHECK(vector().currentState.y == 0 && boolean(activation).currentState);
    CHECK(xrEndSession(session) == XR_SUCCESS);
    CHECK(xrDestroySession(session) == XR_SUCCESS);
    CHECK(xrDestroyInstance(instance) == XR_SUCCESS);
    std::filesystem::remove(argv[5]);
    std::filesystem::remove(std::string(argv[5]) + ".status");
    std::cout << "Khronos loader -> Arm Swing layer -> controlled runtime: vector2, scalar axes, "
                 "remapping and fail-safe checks passed\n";
}
