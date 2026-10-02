// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <armswing/control.h>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <openxr/openxr.h>
#include <openxr/openxr_loader_negotiation.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace layer {
using namespace armswing;
constexpr const char* Name = "XR_APILAYER_ARMSWING_locomotion";
#define DOWN_FUNCTIONS(F)                                                                          \
    F(DestroyInstance)                                                                             \
    F(CreateActionSet)                                                                             \
    F(DestroyActionSet) F(CreateAction) F(DestroyAction) F(StringToPath) F(PathToString)           \
        F(SuggestInteractionProfileBindings) F(CreateSession) F(DestroySession)                    \
            F(CreateReferenceSpace) F(CreateActionSpace) F(AttachSessionActionSets) F(SyncActions) \
                F(GetActionStateBoolean) F(GetActionStateFloat) F(GetActionStateVector2f)          \
                    F(GetActionStatePose) F(LocateSpace) F(WaitFrame)                              \
                        F(EnumerateBoundSourcesForAction)
struct Instance {
    XrInstance handle = XR_NULL_HANDLE;
    PFN_xrGetInstanceProcAddr get = nullptr;
#define FIELD(name) PFN_xr##name name = nullptr;
    DOWN_FUNCTIONS(FIELD)
#undef FIELD
    XrActionSet set = XR_NULL_HANDLE;
    XrAction a = XR_NULL_HANDLE, b = XR_NULL_HANDLE, stick = XR_NULL_HANDLE, pose = XR_NULL_HANDLE;
    XrPath hands[2]{};
    XrPath indexProfile = XR_NULL_PATH;
    bool ready = false;
    std::vector<XrActionSuggestedBinding> bindings;
    bool initialize() {
        bool complete = true;
#define LOAD(name)                                                                                 \
    if (get(handle, "xr" #name, reinterpret_cast<PFN_xrVoidFunction*>(&name)) != XR_SUCCESS ||     \
        !name)                                                                                     \
        complete = false;
        DOWN_FUNCTIONS(LOAD)
#undef LOAD
        if (!complete)
            return false;
        XrActionSetCreateInfo ci{};
        ci.type = XR_TYPE_ACTION_SET_CREATE_INFO;
        std::strcpy(ci.actionSetName, "armswing_private");
        std::strcpy(ci.localizedActionSetName, "Arm Swing VR");
        if (CreateActionSet(handle, &ci, &set) != XR_SUCCESS)
            return false;
        if (StringToPath(handle, "/user/hand/left", &hands[0]) != XR_SUCCESS ||
            StringToPath(handle, "/user/hand/right", &hands[1]) != XR_SUCCESS ||
            StringToPath(handle, "/interaction_profiles/valve/index_controller", &indexProfile) !=
                XR_SUCCESS)
            return false;
        auto create = [&](const char* name, XrActionType type, XrAction& action) {
            XrActionCreateInfo info{};
            info.type = XR_TYPE_ACTION_CREATE_INFO;
            std::strcpy(info.actionName, name);
            std::strcpy(info.localizedActionName, name);
            info.actionType = type;
            info.countSubactionPaths = 2;
            info.subactionPaths = hands;
            return CreateAction(set, &info, &action) == XR_SUCCESS;
        };
        if (!create("armswing_a", XR_ACTION_TYPE_BOOLEAN_INPUT, a) ||
            !create("armswing_b", XR_ACTION_TYPE_BOOLEAN_INPUT, b) ||
            !create("armswing_stick", XR_ACTION_TYPE_VECTOR2F_INPUT, stick) ||
            !create("armswing_pose", XR_ACTION_TYPE_POSE_INPUT, pose))
            return false;
        for (unsigned hand = 0; hand < 2; ++hand) {
            const std::string prefix = hand ? "/user/hand/right/input/" : "/user/hand/left/input/";
            for (const auto& pair : {std::pair{a, "a/click"},
                                     {b, "b/click"},
                                     {stick, "thumbstick"},
                                     {pose, "grip/pose"}}) {
                XrPath path{};
                if (StringToPath(handle, (prefix + pair.second).c_str(), &path) != XR_SUCCESS)
                    return false;
                bindings.push_back({pair.first, path});
            }
        }
        XrInteractionProfileSuggestedBinding suggestion{};
        suggestion.type = XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING;
        suggestion.interactionProfile = indexProfile;
        suggestion.countSuggestedBindings = uint32_t(bindings.size());
        suggestion.suggestedBindings = bindings.data();
        return SuggestInteractionProfileBindings(handle, &suggestion) == XR_SUCCESS;
    }
};
struct Key {
    XrAction action;
    XrPath subaction;
    bool operator<(const Key& b) const {
        return std::pair{action, subaction} < std::pair{b.action, b.subaction};
    }
};
struct Cached {
    unsigned kind = 0;
    uint64_t frame = 0;
    float x = 0, y = 0;
    XrBool32 active = XR_FALSE, changed = XR_FALSE;
    XrTime changeTime = 0;
};
struct Session {
    std::shared_ptr<Instance> instance;
    std::recursive_mutex mutex;
    XrSession handle = XR_NULL_HANDLE;
    XrSpace head = XR_NULL_HANDLE, base = XR_NULL_HANDLE, hands[2]{};
    bool spaces = false, attached = false;
    XrTime time = 0;
    uint64_t frame = 0;
    ControlPacket control{};
    MotionProcessor processor;
    Output output;
    std::map<Key, Cached> cache;
    bool createSpaces() {
        auto& i = *instance;
        XrReferenceSpaceCreateInfo info{};
        info.type = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
        info.poseInReferenceSpace.orientation.w = 1;
        info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        if (i.CreateReferenceSpace(handle, &info, &base) != XR_SUCCESS)
            return false;
        info.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        if (i.CreateReferenceSpace(handle, &info, &head) != XR_SUCCESS)
            return false;
        for (unsigned hand = 0; hand < 2; ++hand) {
            XrActionSpaceCreateInfo ci{};
            ci.type = XR_TYPE_ACTION_SPACE_CREATE_INFO;
            ci.action = i.pose;
            ci.subactionPath = i.hands[hand];
            ci.poseInActionSpace.orientation.w = 1;
            if (i.CreateActionSpace(handle, &ci, &hands[hand]) != XR_SUCCESS)
                return false;
        }
        return true;
    }
    Pose locate(XrSpace space) {
        Pose result;
        if (time <= 0 || !space)
            return result;
        XrSpaceLocation location{};
        location.type = XR_TYPE_SPACE_LOCATION;
        if (instance->LocateSpace(space, base, time, &location) != XR_SUCCESS)
            return result;
        constexpr auto valid =
            XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT |
            XR_SPACE_LOCATION_POSITION_TRACKED_BIT | XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
        result.valid = (location.locationFlags & valid) == valid;
        result.position = {location.pose.position.x, location.pose.position.y,
                           location.pose.position.z};
        const auto q = location.pose.orientation;
        result.forward = {-2 * (q.x * q.z + q.w * q.y), -(1 - 2 * (q.x * q.x + q.y * q.y))};
        return result;
    }
    void sample(bool enabled) {
        Sample s;
        s.timeNs = time;
        s.focused = enabled;
        if (enabled) {
            s.head = locate(head);
            auto& i = *instance;
            for (unsigned hand = 0; hand < 2; ++hand) {
                XrActionStateGetInfo get{};
                get.type = XR_TYPE_ACTION_STATE_GET_INFO;
                get.subactionPath = i.hands[hand];
                get.action = i.pose;
                XrActionStatePose poseState{};
                poseState.type = XR_TYPE_ACTION_STATE_POSE;
                if (i.GetActionStatePose(handle, &get, &poseState) == XR_SUCCESS &&
                    poseState.isActive)
                    s.hands[hand] = locate(hands[hand]);
                for (unsigned button = 0; button < 2; ++button) {
                    get.action = button ? i.b : i.a;
                    XrActionStateBoolean state{};
                    state.type = XR_TYPE_ACTION_STATE_BOOLEAN;
                    if (i.GetActionStateBoolean(handle, &get, &state) == XR_SUCCESS &&
                        state.isActive)
                        s.buttons[hand * 2 + button] = state.currentState;
                    else if (hand * 2 + button == control.motion.activation)
                        s.focused = false;
                }
            }
        }
        output = processor.update(control.motion, s, enabled);
        reportStatus(Backend::OpenXr, output);
    }
    std::vector<std::string> paths(const XrActionStateGetInfo& get) {
        auto& i = *instance;
        XrBoundSourcesForActionEnumerateInfo info{};
        info.type = XR_TYPE_BOUND_SOURCES_FOR_ACTION_ENUMERATE_INFO;
        info.action = get.action;
        uint32_t count = 0;
        if (i.EnumerateBoundSourcesForAction(handle, &info, 0, &count, nullptr) != XR_SUCCESS ||
            count > 64)
            return {};
        std::vector<XrPath> values(count);
        if (i.EnumerateBoundSourcesForAction(handle, &info, count, &count, values.data()) !=
                XR_SUCCESS ||
            count > values.size())
            return {};
        std::vector<std::string> result;
        std::string prefix;
        if (get.subactionPath) {
            char text[XR_MAX_PATH_LENGTH]{};
            uint32_t length{};
            if (i.PathToString(i.handle, get.subactionPath, sizeof(text), &length, text) !=
                XR_SUCCESS)
                return {};
            prefix = std::string(text) + "/";
        }
        for (uint32_t index = 0; index < count; ++index) {
            char text[XR_MAX_PATH_LENGTH]{};
            uint32_t length{};
            if (i.PathToString(i.handle, values[index], sizeof(text), &length, text) != XR_SUCCESS)
                return {};
            if (prefix.empty() || std::string(text).starts_with(prefix))
                result.emplace_back(text);
        }
        return result;
    }
    Cached remember(const XrActionStateGetInfo& get, float x, float y, XrBool32 active,
                    XrBool32 changed, XrTime changeTime, bool altered, unsigned kind) {
        const Key key{get.action, get.subactionPath};
        const auto found = cache.find(key);
        if (found != cache.end() && found->second.frame == frame)
            return found->second;
        if (active && found != cache.end()) {
            const auto& previous = found->second;
            changed = previous.active && (previous.x != x || previous.y != y);
            if (changed)
                changeTime = time > 0 ? time : changeTime;
            else
                changeTime = previous.changeTime;
        } else if (active && altered) {
            changed = XR_TRUE;
            changeTime = time > 0 ? time : changeTime;
        }
        const Cached value{kind, frame, x, y, active, changed, changeTime};
        cache[key] = value;
        return value;
    }
};
static std::mutex registryMutex;
static std::map<XrInstance, std::shared_ptr<Instance>> instances;
static std::map<XrSession, std::shared_ptr<Session>> sessions;
static std::map<XrActionSet, std::shared_ptr<Instance>> actionSets;
static std::map<XrAction, XrActionSet> appActions;
static std::shared_ptr<Instance> instance(XrInstance h) {
    std::lock_guard lock(registryMutex);
    const auto it = instances.find(h);
    return it == instances.end() ? nullptr : it->second;
}
static std::shared_ptr<Session> session(XrSession h) {
    std::lock_guard lock(registryMutex);
    const auto it = sessions.find(h);
    return it == sessions.end() ? nullptr : it->second;
}
XrResult XRAPI_CALL xrDestroyInstance(XrInstance h) {
    const auto i = instance(h);
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->DestroyInstance(h);
    if (XR_SUCCEEDED(result)) {
        std::lock_guard lock(registryMutex);
        std::erase_if(sessions, [&](const auto& pair) { return pair.second->instance == i; });
        std::erase_if(appActions, [&](const auto& pair) {
            return actionSets.contains(pair.second) && actionSets[pair.second] == i;
        });
        std::erase_if(actionSets, [&](const auto& pair) { return pair.second == i; });
        instances.erase(h);
    }
    return result;
}
XrResult XRAPI_CALL xrCreateActionSet(XrInstance h, const XrActionSetCreateInfo* info,
                                      XrActionSet* handle) {
    const auto i = instance(h);
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->CreateActionSet(h, info, handle);
    if (result == XR_SUCCESS) {
        std::lock_guard lock(registryMutex);
        actionSets[*handle] = i;
    }
    return result;
}
XrResult XRAPI_CALL xrCreateAction(XrActionSet set, const XrActionCreateInfo* info,
                                   XrAction* handle) {
    std::shared_ptr<Instance> i;
    {
        std::lock_guard lock(registryMutex);
        const auto it = actionSets.find(set);
        if (it != actionSets.end())
            i = it->second;
    }
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->CreateAction(set, info, handle);
    if (result == XR_SUCCESS) {
        std::lock_guard lock(registryMutex);
        appActions[*handle] = set;
    }
    return result;
}
static void clearActionCaches(const std::vector<XrAction>& actions) {
    std::vector<std::shared_ptr<Session>> targets;
    {
        std::lock_guard lock(registryMutex);
        for (const auto& [_, s] : sessions)
            targets.push_back(s);
    }
    for (const auto& s : targets) {
        std::lock_guard lock(s->mutex);
        std::erase_if(s->cache, [&](const auto& pair) {
            return std::find(actions.begin(), actions.end(), pair.first.action) != actions.end();
        });
    }
}
XrResult XRAPI_CALL xrDestroyAction(XrAction action) {
    std::shared_ptr<Instance> i;
    {
        std::lock_guard lock(registryMutex);
        const auto a = appActions.find(action);
        if (a != appActions.end())
            i = actionSets[a->second];
    }
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->DestroyAction(action);
    if (result == XR_SUCCESS) {
        clearActionCaches({action});
        std::lock_guard lock(registryMutex);
        appActions.erase(action);
    }
    return result;
}
XrResult XRAPI_CALL xrDestroyActionSet(XrActionSet set) {
    std::shared_ptr<Instance> i;
    {
        std::lock_guard lock(registryMutex);
        const auto it = actionSets.find(set);
        if (it != actionSets.end())
            i = it->second;
    }
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->DestroyActionSet(set);
    if (result == XR_SUCCESS) {
        std::vector<XrAction> removed;
        {
            std::lock_guard lock(registryMutex);
            for (const auto& [action, owner] : appActions)
                if (owner == set)
                    removed.push_back(action);
            for (const auto a : removed)
                appActions.erase(a);
            actionSets.erase(set);
        }
        clearActionCaches(removed);
    }
    return result;
}
XrResult XRAPI_CALL xrCreateSession(XrInstance h, const XrSessionCreateInfo* info,
                                    XrSession* handle) {
    const auto i = instance(h);
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->CreateSession(h, info, handle);
    if (result == XR_SUCCESS) {
        auto s = std::make_shared<Session>();
        s->instance = i;
        s->handle = *handle;
        s->spaces = i->ready && s->createSpaces();
        std::lock_guard lock(registryMutex);
        sessions[*handle] = s;
    }
    return result;
}
XrResult XRAPI_CALL xrDestroySession(XrSession h) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = s->instance->DestroySession(h);
    if (XR_SUCCEEDED(result)) {
        std::lock_guard lock(registryMutex);
        sessions.erase(h);
    }
    return result;
}
XrResult XRAPI_CALL xrSuggestInteractionProfileBindings(
    XrInstance h, const XrInteractionProfileSuggestedBinding* info) {
    const auto i = instance(h);
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    if (!info || !i->ready || info->interactionProfile != i->indexProfile)
        return i->SuggestInteractionProfileBindings(h, info);
    auto merged = *info;
    std::vector<XrActionSuggestedBinding> bindings(
        info->suggestedBindings, info->suggestedBindings + info->countSuggestedBindings);
    bindings.insert(bindings.end(), i->bindings.begin(), i->bindings.end());
    merged.countSuggestedBindings = uint32_t(bindings.size());
    merged.suggestedBindings = bindings.data();
    return i->SuggestInteractionProfileBindings(h, &merged);
}
XrResult XRAPI_CALL xrAttachSessionActionSets(XrSession h,
                                              const XrSessionActionSetsAttachInfo* info) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    std::lock_guard lock(s->mutex);
    if (!s->spaces || !info || !info->countActionSets)
        return s->instance->AttachSessionActionSets(h, info);
    auto merged = *info;
    std::vector<XrActionSet> sets(info->actionSets, info->actionSets + info->countActionSets);
    sets.push_back(s->instance->set);
    merged.countActionSets = uint32_t(sets.size());
    merged.actionSets = sets.data();
    const auto result = s->instance->AttachSessionActionSets(h, &merged);
    if (result == XR_SUCCESS)
        s->attached = true;
    return result;
}
XrResult XRAPI_CALL xrWaitFrame(XrSession h, const XrFrameWaitInfo* info, XrFrameState* state) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = s->instance->WaitFrame(h, info, state);
    if (result == XR_SUCCESS) {
        std::lock_guard lock(s->mutex);
        s->time = state->predictedDisplayTime;
    }
    return result;
}
XrResult XRAPI_CALL xrGetActionStateBoolean(XrSession, const XrActionStateGetInfo*,
                                            XrActionStateBoolean*);
XrResult XRAPI_CALL xrGetActionStateFloat(XrSession, const XrActionStateGetInfo*,
                                          XrActionStateFloat*);
XrResult XRAPI_CALL xrGetActionStateVector2f(XrSession, const XrActionStateGetInfo*,
                                             XrActionStateVector2f*);
XrResult XRAPI_CALL xrSyncActions(XrSession h, const XrActionsSyncInfo* info) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    std::lock_guard lock(s->mutex);
    ControlPacket packet;
    const bool enabled =
        info && info->countActiveActionSets && s->attached && readControl(Backend::OpenXr, packet);
    XrResult result;
    if (enabled) {
        if (s->control.generation != packet.generation)
            s->processor.reset();
        s->control = packet;
        auto merged = *info;
        std::vector<XrActiveActionSet> sets(info->activeActionSets,
                                            info->activeActionSets + info->countActiveActionSets);
        sets.push_back({s->instance->set, XR_NULL_PATH});
        merged.countActiveActionSets = uint32_t(sets.size());
        merged.activeActionSets = sets.data();
        result = s->instance->SyncActions(h, &merged);
    } else
        result = s->instance->SyncActions(h, info);
    ++s->frame;
    s->sample(enabled && result == XR_SUCCESS);
    for (const auto& [key, cached] : s->cache) {
        XrActionStateGetInfo get{};
        get.type = XR_TYPE_ACTION_STATE_GET_INFO;
        get.action = key.action;
        get.subactionPath = key.subaction;
        if (cached.kind == 0) {
            XrActionStateBoolean state{};
            state.type = XR_TYPE_ACTION_STATE_BOOLEAN;
            layer::xrGetActionStateBoolean(h, &get, &state);
        } else if (cached.kind == 1) {
            XrActionStateFloat state{};
            state.type = XR_TYPE_ACTION_STATE_FLOAT;
            layer::xrGetActionStateFloat(h, &get, &state);
        } else {
            XrActionStateVector2f state{};
            state.type = XR_TYPE_ACTION_STATE_VECTOR2F;
            layer::xrGetActionStateVector2f(h, &get, &state);
        }
    }
    return result;
}
XrResult XRAPI_CALL xrGetActionStateBoolean(XrSession h, const XrActionStateGetInfo* info,
                                            XrActionStateBoolean* state) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    std::lock_guard lock(s->mutex);
    const auto result = s->instance->GetActionStateBoolean(h, info, state);
    if (result != XR_SUCCESS)
        return result;
    const auto before = state->currentState;
    if (s->output.ownsInput && state->isActive) {
        const auto paths = s->paths(*info);
        bool supported = !paths.empty(), pressed = false;
        for (const auto& path : paths) {
            const auto index = buttonIndex(path);
            if (index < 0) {
                supported = false;
                break;
            }
            pressed |= s->output.buttons[index];
        }
        if (supported)
            state->currentState = pressed;
    }
    const auto cached = s->remember(*info, float(state->currentState), 0, state->isActive,
                                    state->changedSinceLastSync, state->lastChangeTime,
                                    state->currentState != before, 0);
    state->currentState = cached.x != 0;
    state->changedSinceLastSync = cached.changed;
    state->lastChangeTime = cached.changeTime;
    return result;
}
static int movementAxis(const std::string& path, unsigned hand) {
    const std::string base =
        std::string("/user/hand/") + (hand ? "right" : "left") + "/input/thumbstick";
    if (path == base)
        return 2;
    if (path == base + "/x")
        return 0;
    if (path == base + "/y")
        return 1;
    return -1;
}
XrResult XRAPI_CALL xrGetActionStateVector2f(XrSession h, const XrActionStateGetInfo* info,
                                             XrActionStateVector2f* state) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    std::lock_guard lock(s->mutex);
    const auto result = s->instance->GetActionStateVector2f(h, info, state);
    if (result != XR_SUCCESS)
        return result;
    const auto before = state->currentState;
    if (s->output.ownsInput && state->isActive) {
        const auto paths = s->paths(*info);
        if (!paths.empty() && std::all_of(paths.begin(), paths.end(), [&](const auto& p) {
                return movementAxis(p, s->control.motion.outputHand) == 2;
            })) {
            const auto v = applyMovement({before.x, before.y}, s->output);
            state->currentState = {v.x, v.y};
        }
    }
    const auto cached =
        s->remember(*info, state->currentState.x, state->currentState.y, state->isActive,
                    state->changedSinceLastSync, state->lastChangeTime,
                    state->currentState.x != before.x || state->currentState.y != before.y, 2);
    state->currentState = {cached.x, cached.y};
    state->changedSinceLastSync = cached.changed;
    state->lastChangeTime = cached.changeTime;
    return result;
}
XrResult XRAPI_CALL xrGetActionStateFloat(XrSession h, const XrActionStateGetInfo* info,
                                          XrActionStateFloat* state) {
    const auto s = session(h);
    if (!s)
        return XR_ERROR_HANDLE_INVALID;
    std::lock_guard lock(s->mutex);
    const auto result = s->instance->GetActionStateFloat(h, info, state);
    if (result != XR_SUCCESS)
        return result;
    const float before = state->currentState;
    if (s->output.ownsInput && state->isActive) {
        const auto paths = s->paths(*info);
        const int axis =
            paths.empty() ? -1 : movementAxis(paths.front(), s->control.motion.outputHand);
        if ((axis == 0 || axis == 1) && std::all_of(paths.begin(), paths.end(), [&](const auto& p) {
                return movementAxis(p, s->control.motion.outputHand) == axis;
            })) {
            // Query the complete physical stick so X/Y actions share manual-stick precedence.
            XrActionStateGetInfo get{};
            get.type = XR_TYPE_ACTION_STATE_GET_INFO;
            get.action = s->instance->stick;
            get.subactionPath = s->instance->hands[s->control.motion.outputHand];
            XrActionStateVector2f stick{};
            stick.type = XR_TYPE_ACTION_STATE_VECTOR2F;
            if (s->instance->GetActionStateVector2f(h, &get, &stick) == XR_SUCCESS &&
                stick.isActive) {
                const Vec2 physical{stick.currentState.x, stick.currentState.y};
                const auto v = applyMovement(physical, s->output);
                if (v.x != physical.x || v.y != physical.y)
                    state->currentState = axis ? v.y : v.x;
            }
        }
    }
    const auto cached =
        s->remember(*info, state->currentState, 0, state->isActive, state->changedSinceLastSync,
                    state->lastChangeTime, state->currentState != before, 1);
    state->currentState = cached.x;
    state->changedSinceLastSync = cached.changed;
    state->lastChangeTime = cached.changeTime;
    return result;
}
XrResult XRAPI_CALL getProc(XrInstance h, const char* name, PFN_xrVoidFunction* function) {
    if (!name || !function)
        return XR_ERROR_VALIDATION_FAILURE;
    *function = nullptr;
    const auto i = instance(h);
    if (!i)
        return XR_ERROR_HANDLE_INVALID;
    const auto result = i->get(h, name, function);
    if (result != XR_SUCCESS)
        return result;
#define WRAP(name_)                                                                                \
    if (std::strcmp(name, #name_) == 0)                                                            \
        *function = reinterpret_cast<PFN_xrVoidFunction>(&layer::name_);
    WRAP(xrDestroyInstance)
    WRAP(xrCreateSession)
    WRAP(xrDestroySession) WRAP(xrCreateActionSet) WRAP(xrCreateAction) WRAP(xrDestroyActionSet)
        WRAP(xrDestroyAction) WRAP(xrSuggestInteractionProfileBindings)
            WRAP(xrAttachSessionActionSets) WRAP(xrWaitFrame) WRAP(xrSyncActions)
                WRAP(xrGetActionStateBoolean) WRAP(xrGetActionStateFloat)
                    WRAP(xrGetActionStateVector2f)
#undef WRAP
                        if (std::strcmp(name, "xrGetInstanceProcAddr") == 0)* function =
                            reinterpret_cast<PFN_xrVoidFunction>(getProc);
    return result;
}
XrResult XRAPI_CALL create(const XrInstanceCreateInfo* info, const XrApiLayerCreateInfo* layerInfo,
                           XrInstance* handle) {
    if (!info || !layerInfo || !layerInfo->nextInfo || !handle ||
        layerInfo->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_CREATE_INFO ||
        layerInfo->structVersion != XR_API_LAYER_CREATE_INFO_STRUCT_VERSION ||
        layerInfo->structSize != sizeof(XrApiLayerCreateInfo) ||
        !layerInfo->nextInfo->nextGetInstanceProcAddr ||
        !layerInfo->nextInfo->nextCreateApiLayerInstance)
        return XR_ERROR_INITIALIZATION_FAILED;
    auto next = *layerInfo;
    next.nextInfo = layerInfo->nextInfo->next;
    const auto result = layerInfo->nextInfo->nextCreateApiLayerInstance(info, &next, handle);
    if (result != XR_SUCCESS)
        return result;
    try {
        auto i = std::make_shared<Instance>();
        i->handle = *handle;
        i->get = layerInfo->nextInfo->nextGetInstanceProcAddr;
        i->ready = i->initialize();
        std::lock_guard lock(registryMutex);
        instances[*handle] = i;
        return XR_SUCCESS;
    } catch (...) {
        PFN_xrDestroyInstance destroy = nullptr;
        layerInfo->nextInfo->nextGetInstanceProcAddr(
            *handle, "xrDestroyInstance", reinterpret_cast<PFN_xrVoidFunction*>(&destroy));
        if (destroy)
            destroy(*handle);
        *handle = XR_NULL_HANDLE;
        return XR_ERROR_OUT_OF_MEMORY;
    }
}
} // namespace layer
EXPORT XrResult XRAPI_CALL xrNegotiateLoaderApiLayerInterface(const XrNegotiateLoaderInfo* loader,
                                                              const char* name,
                                                              XrNegotiateApiLayerRequest* request) {
    if (!loader || !request || !name || std::strcmp(name, layer::Name) != 0 ||
        loader->structType != XR_LOADER_INTERFACE_STRUCT_LOADER_INFO ||
        loader->structVersion != XR_LOADER_INFO_STRUCT_VERSION ||
        loader->structSize != sizeof(*loader) ||
        request->structType != XR_LOADER_INTERFACE_STRUCT_API_LAYER_REQUEST ||
        request->structVersion != XR_API_LAYER_INFO_STRUCT_VERSION ||
        request->structSize != sizeof(*request) || loader->minInterfaceVersion > 1 ||
        loader->maxInterfaceVersion < 1 || loader->maxApiVersion < XR_MAKE_VERSION(1, 0, 0))
        return XR_ERROR_INITIALIZATION_FAILED;
    request->layerInterfaceVersion = 1;
    request->layerApiVersion = XR_MAKE_VERSION(1, 0, 0);
    request->getInstanceProcAddr = layer::getProc;
    request->createApiLayerInstance = layer::create;
    return XR_SUCCESS;
}
