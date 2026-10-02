// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
// Controlled test runtime. Never registered as the user's active runtime.
#include "fixture.h"
#include <algorithm>
#include <cstring>
#include <map>
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <string>
#include <vector>
#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif
namespace fixture {
static XrFixtureFrame frame;
static XrFixtureStats stats;
static uintptr_t serial = 1000;
template <class T> T handle() { return reinterpret_cast<T>(++serial); }
static XrInstance instance;
static XrSession session;
static XrTime time = 1000000000;
struct Action {
    XrActionSet set;
    XrActionType type;
    std::string name;
    std::vector<XrPath> sources;
};
static std::map<XrAction, Action> actions;
static std::map<XrActionSet, std::string> sets;
static std::map<XrSpace, int> spaces;
static std::map<std::string, XrPath> paths;
static std::map<XrPath, std::string> strings;
static std::vector<XrActionSet> active;
XrResult XRAPI_CALL get(XrInstance, const char*, PFN_xrVoidFunction*);
static XrPath path(const std::string& text) {
    if (!paths.contains(text)) {
        const auto id = XrPath(paths.size() + 1);
        paths[text] = id;
        strings[id] = text;
    }
    return paths[text];
}
XrResult XRAPI_CALL xrEnumerateInstanceExtensionProperties(const char*, uint32_t capacity,
                                                           uint32_t* count,
                                                           XrExtensionProperties* properties) {
    *count = 1;
    if (capacity) {
        properties[0] = {};
        properties[0].type = XR_TYPE_EXTENSION_PROPERTIES;
        std::strcpy(properties[0].extensionName, "XR_MND_headless");
        properties[0].extensionVersion = 2;
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrCreateInstance(const XrInstanceCreateInfo*, XrInstance* out) {
    actions.clear();
    sets.clear();
    spaces.clear();
    active.clear();
    stats = {};
    instance = handle<XrInstance>();
    *out = instance;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrDestroyInstance(XrInstance) {
    actions.clear();
    sets.clear();
    spaces.clear();
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrGetInstanceProperties(XrInstance, XrInstanceProperties* p) {
    p->runtimeVersion = XR_MAKE_VERSION(1, 0, 0);
    std::strcpy(p->runtimeName, "Arm Swing integration fixture");
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrGetSystem(XrInstance, const XrSystemGetInfo*, XrSystemId* id) {
    *id = 1;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrCreateActionSet(XrInstance, const XrActionSetCreateInfo* info,
                                      XrActionSet* out) {
    *out = handle<XrActionSet>();
    sets[*out] = info->actionSetName;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet set) {
    sets.erase(set);
    std::erase_if(actions, [&](const auto& pair) { return pair.second.set == set; });
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrCreateAction(XrActionSet set, const XrActionCreateInfo* info, XrAction* out) {
    *out = handle<XrAction>();
    actions[*out] = {set, info->actionType, info->actionName, {}};
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrDestroyAction(XrAction a) {
    actions.erase(a);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrStringToPath(XrInstance, const char* text, XrPath* out) {
    *out = path(text);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrPathToString(XrInstance, XrPath p, uint32_t capacity, uint32_t* count,
                                   char* text) {
    if (!strings.contains(p))
        return XR_ERROR_PATH_INVALID;
    const auto& s = strings[p];
    *count = uint32_t(s.size() + 1);
    if (capacity == 0)
        return XR_SUCCESS;
    if (capacity < *count)
        return XR_ERROR_SIZE_INSUFFICIENT;
    std::strcpy(text, s.c_str());
    return XR_SUCCESS;
}
XrResult XRAPI_CALL
xrSuggestInteractionProfileBindings(XrInstance, const XrInteractionProfileSuggestedBinding* info) {
    for (auto& [_, action] : actions)
        action.sources.clear();
    stats.suggestions = info->countSuggestedBindings;
    for (unsigned i = 0; i < info->countSuggestedBindings; ++i) {
        const auto& binding = info->suggestedBindings[i];
        if (!actions.contains(binding.action))
            return XR_ERROR_HANDLE_INVALID;
        actions[binding.action].sources.push_back(binding.binding);
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrCreateSession(XrInstance, const XrSessionCreateInfo*, XrSession* out) {
    session = handle<XrSession>();
    *out = session;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrDestroySession(XrSession) {
    spaces.clear();
    active.clear();
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrCreateReferenceSpace(XrSession, const XrReferenceSpaceCreateInfo* info,
                                           XrSpace* out) {
    *out = handle<XrSpace>();
    spaces[*out] = info->referenceSpaceType == XR_REFERENCE_SPACE_TYPE_VIEW ? 0 : 3;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrCreateActionSpace(XrSession, const XrActionSpaceCreateInfo* info,
                                        XrSpace* out) {
    *out = handle<XrSpace>();
    spaces[*out] = strings[info->subactionPath].find("left") != std::string::npos ? 1 : 2;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrDestroySpace(XrSpace s) {
    spaces.erase(s);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrLocateSpace(XrSpace s, XrSpace, XrTime, XrSpaceLocation* out) {
    if (!spaces.contains(s))
        return XR_ERROR_HANDLE_INVALID;
    out->locationFlags = frame.tracked ? XR_SPACE_LOCATION_POSITION_VALID_BIT |
                                             XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
                                             XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
                                             XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT
                                       : 0;
    out->pose = {};
    out->pose.orientation.w = 1;
    const auto role = spaces[s];
    out->pose.position.x = role == 1 ? -.3f : role == 2 ? .3f : 0;
    out->pose.position.y = role == 0 ? 1.7f : 1 + (role == 1 ? frame.swing : -frame.swing);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession,
                                              const XrSessionActionSetsAttachInfo* info) {
    stats.attachedSets = info->countActionSets;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrSyncActions(XrSession, const XrActionsSyncInfo* info) {
    stats.syncedSets = info->countActiveActionSets;
    active.clear();
    for (unsigned i = 0; i < info->countActiveActionSets; ++i)
        active.push_back(info->activeActionSets[i].actionSet);
    return frame.focused ? XR_SUCCESS : XR_SESSION_NOT_FOCUSED;
}
static bool isActive(XrAction action, XrPath subaction) {
    if (!actions.contains(action) || !frame.focused || !frame.actionActive)
        return false;
    const auto& a = actions[action];
    if (std::find(active.begin(), active.end(), a.set) == active.end())
        return false;
    return std::any_of(a.sources.begin(), a.sources.end(), [&](XrPath p) {
        return !subaction || strings[p].starts_with(strings[subaction] + "/");
    });
}
XrResult XRAPI_CALL xrGetActionStateBoolean(XrSession, const XrActionStateGetInfo* info,
                                            XrActionStateBoolean* out) {
    if (!actions.contains(info->action))
        return XR_ERROR_HANDLE_INVALID;
    if (actions[info->action].type != XR_ACTION_TYPE_BOOLEAN_INPUT)
        return XR_ERROR_ACTION_TYPE_MISMATCH;
    out->isActive = isActive(info->action, info->subactionPath);
    out->currentState = XR_FALSE;
    out->changedSinceLastSync = XR_FALSE;
    out->lastChangeTime = 1000000000;
    if (out->isActive) {
        for (const auto p : actions[info->action].sources) {
            const auto& s = strings[p];
            if (info->subactionPath && !s.starts_with(strings[info->subactionPath] + "/"))
                continue;
            if (s == "/user/hand/left/input/a/click")
                out->currentState |= frame.activation;
            if (s.ends_with("/trigger/click"))
                out->currentState = XR_TRUE;
        }
    }
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrGetActionStateVector2f(XrSession, const XrActionStateGetInfo* info,
                                             XrActionStateVector2f* out) {
    if (!actions.contains(info->action))
        return XR_ERROR_HANDLE_INVALID;
    if (actions[info->action].type != XR_ACTION_TYPE_VECTOR2F_INPUT)
        return XR_ERROR_ACTION_TYPE_MISMATCH;
    out->isActive = isActive(info->action, info->subactionPath);
    out->currentState = out->isActive ? XrVector2f{frame.manualX, frame.manualY} : XrVector2f{};
    out->changedSinceLastSync = XR_FALSE;
    out->lastChangeTime = 1000000000;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrGetActionStateFloat(XrSession, const XrActionStateGetInfo* info,
                                          XrActionStateFloat* out) {
    if (!actions.contains(info->action))
        return XR_ERROR_HANDLE_INVALID;
    if (actions[info->action].type != XR_ACTION_TYPE_FLOAT_INPUT)
        return XR_ERROR_ACTION_TYPE_MISMATCH;
    out->isActive = isActive(info->action, info->subactionPath);
    out->currentState = 0;
    if (out->isActive && !actions[info->action].sources.empty())
        out->currentState = strings[actions[info->action].sources[0]].ends_with("/y")
                                ? frame.manualY
                                : frame.manualX;
    out->changedSinceLastSync = XR_FALSE;
    out->lastChangeTime = 1000000000;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrGetActionStatePose(XrSession, const XrActionStateGetInfo* info,
                                         XrActionStatePose* out) {
    if (!actions.contains(info->action))
        return XR_ERROR_HANDLE_INVALID;
    out->isActive = isActive(info->action, info->subactionPath);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL
xrEnumerateBoundSourcesForAction(XrSession, const XrBoundSourcesForActionEnumerateInfo* info,
                                 uint32_t capacity, uint32_t* count, XrPath* values) {
    if (!actions.contains(info->action))
        return XR_ERROR_HANDLE_INVALID;
    const auto& sources = actions[info->action].sources;
    *count = uint32_t(sources.size());
    if (!capacity)
        return XR_SUCCESS;
    if (capacity < *count)
        return XR_ERROR_SIZE_INSUFFICIENT;
    std::copy(sources.begin(), sources.end(), values);
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrWaitFrame(XrSession, const XrFrameWaitInfo*, XrFrameState* out) {
    time += 11111111;
    out->predictedDisplayTime = time;
    out->predictedDisplayPeriod = 11111111;
    out->shouldRender = XR_TRUE;
    return XR_SUCCESS;
}
XrResult XRAPI_CALL xrBeginFrame(XrSession, const XrFrameBeginInfo*) { return XR_SUCCESS; }
XrResult XRAPI_CALL xrEndFrame(XrSession, const XrFrameEndInfo*) { return XR_SUCCESS; }
XrResult XRAPI_CALL xrBeginSession(XrSession, const XrSessionBeginInfo*) { return XR_SUCCESS; }
XrResult XRAPI_CALL xrEndSession(XrSession) { return XR_SUCCESS; }
XrResult XRAPI_CALL xrPollEvent(XrInstance, XrEventDataBuffer*) { return XR_EVENT_UNAVAILABLE; }
XrResult XRAPI_CALL xrResultToString(XrInstance, XrResult, char text[XR_MAX_RESULT_STRING_SIZE]) {
    std::strcpy(text, "FixtureResult");
    return XR_SUCCESS;
}
XrResult XRAPI_CALL get(XrInstance, const char* name, PFN_xrVoidFunction* function) {
    *function = nullptr;
    if (std::strcmp(name, "xrGetInstanceProcAddr") == 0) {
        *function = reinterpret_cast<PFN_xrVoidFunction>(get);
        return XR_SUCCESS;
    }
#define RETURN(fn)                                                                                 \
    if (std::strcmp(name, #fn) == 0) {                                                             \
        *function = reinterpret_cast<PFN_xrVoidFunction>(fixture::fn);                             \
        return XR_SUCCESS;                                                                         \
    }
    RETURN(xrEnumerateInstanceExtensionProperties)
    RETURN(xrCreateInstance)
    RETURN(xrDestroyInstance) RETURN(xrGetInstanceProperties) RETURN(xrGetSystem)
        RETURN(xrCreateActionSet) RETURN(xrDestroyActionSet) RETURN(xrCreateAction)
            RETURN(xrDestroyAction) RETURN(xrStringToPath) RETURN(xrPathToString)
                RETURN(xrSuggestInteractionProfileBindings) RETURN(xrCreateSession)
                    RETURN(xrDestroySession) RETURN(xrCreateReferenceSpace)
                        RETURN(xrCreateActionSpace) RETURN(xrDestroySpace) RETURN(xrLocateSpace)
                            RETURN(xrAttachSessionActionSets) RETURN(xrSyncActions)
                                RETURN(xrGetActionStateBoolean) RETURN(xrGetActionStateFloat)
                                    RETURN(xrGetActionStateVector2f) RETURN(xrGetActionStatePose)
                                        RETURN(xrEnumerateBoundSourcesForAction) RETURN(xrWaitFrame)
                                            RETURN(xrBeginFrame) RETURN(xrEndFrame)
                                                RETURN(xrBeginSession) RETURN(xrEndSession)
                                                    RETURN(xrPollEvent) RETURN(xrResultToString)
#undef RETURN
                                                        return XR_ERROR_FUNCTION_UNSUPPORTED;
}
} // namespace fixture
EXPORT void fixtureSetFrame(const XrFixtureFrame* frame) { fixture::frame = *frame; }
EXPORT void fixtureGetStats(XrFixtureStats* stats) { *stats = fixture::stats; }
EXPORT XrResult XRAPI_CALL xrNegotiateLoaderRuntimeInterface(const XrNegotiateLoaderInfo*,
                                                             XrNegotiateRuntimeRequest* request) {
    request->runtimeInterfaceVersion = 1;
    request->runtimeApiVersion = XR_MAKE_VERSION(1, 0, 0);
    request->getInstanceProcAddr = fixture::get;
    return XR_SUCCESS;
}
