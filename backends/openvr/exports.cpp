// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdlib>
#include <mutex>
#ifdef _WIN32
#include <windows.h>
#define EXPORT extern "C" __declspec(dllexport)
#else
#include <dlfcn.h>
#define EXPORT extern "C" __attribute__((visibility("default")))
#endif
#include <cstdint>

using Factory = void* (*)(const char*, int*);
void* wrapOpenVrCurrent(const char*, void*, Factory);
void* wrapOpenVr22(const char*, void*, Factory);
void* wrapOpenVr19(const char*, void*, Factory);
void resetOpenVrCurrent();
void resetOpenVr22();
void resetOpenVr19();
static void* symbol(const char* name) {
    static std::once_flag once;
#ifdef _WIN32
    static HMODULE library = nullptr;
    std::call_once(once, [] {
        if (const char* path = std::getenv("ARMSWING_OPENVR_REAL"))
            library = LoadLibraryA(path);
    });
    return library ? reinterpret_cast<void*>(GetProcAddress(library, name)) : nullptr;
#else
    static void* library = nullptr;
    std::call_once(once, [] {
        if (const char* path = std::getenv("ARMSWING_OPENVR_REAL"))
            library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    });
    return dlsym(library ? library : RTLD_NEXT, name);
#endif
}
static void* originalFactory(const char* version, int* error) {
    const auto function = reinterpret_cast<Factory>(symbol("VR_GetGenericInterface"));
    if (function)
        return function(version, error);
    if (error)
        *error = 105;
    return nullptr;
}
EXPORT void* VR_GetGenericInterface(const char* version, int* error) {
    void* original = originalFactory(version, error);
    if (!original || !version)
        return original;
    try {
        auto* result = wrapOpenVrCurrent(version, original, originalFactory);
        if (result != original)
            return result;
        result = wrapOpenVr22(version, original, originalFactory);
        if (result != original)
            return result;
        return wrapOpenVr19(version, original, originalFactory);
    } catch (...) {
        return original;
    } // Never terminate a game on an adapter allocation failure.
}
EXPORT uint32_t VR_InitInternal2(int* error, int applicationType, const char* startup) {
    using Function = uint32_t (*)(int*, int, const char*);
    if (const auto f = reinterpret_cast<Function>(symbol("VR_InitInternal2")))
        return f(error, applicationType, startup);
    if (error)
        *error = 105;
    return 0;
}
EXPORT uint32_t VR_InitInternal(int* error, int applicationType) {
    using Function = uint32_t (*)(int*, int);
    if (const auto f = reinterpret_cast<Function>(symbol("VR_InitInternal")))
        return f(error, applicationType);
    return VR_InitInternal2(error, applicationType, nullptr);
}
EXPORT void VR_ShutdownInternal() {
    resetOpenVrCurrent();
    resetOpenVr22();
    resetOpenVr19();
    if (const auto f = reinterpret_cast<void (*)()>(symbol("VR_ShutdownInternal")))
        f();
}
EXPORT bool VR_IsHmdPresent() {
    const auto f = reinterpret_cast<bool (*)()>(symbol("VR_IsHmdPresent"));
    return f && f();
}
EXPORT bool VR_IsRuntimeInstalled() {
    const auto f = reinterpret_cast<bool (*)()>(symbol("VR_IsRuntimeInstalled"));
    return f && f();
}
EXPORT bool VR_IsInterfaceVersionValid(const char* version) {
    const auto f = reinterpret_cast<bool (*)(const char*)>(symbol("VR_IsInterfaceVersionValid"));
    return f && f(version);
}
EXPORT uint32_t VR_GetInitToken() {
    const auto f = reinterpret_cast<uint32_t (*)()>(symbol("VR_GetInitToken"));
    return f ? f() : 0;
}
EXPORT bool VR_GetRuntimePath(char* path, uint32_t size, uint32_t* required) {
    const auto f =
        reinterpret_cast<bool (*)(char*, uint32_t, uint32_t*)>(symbol("VR_GetRuntimePath"));
    return f && f(path, size, required);
}
EXPORT const char* VR_GetVRInitErrorAsEnglishDescription(int error) {
    const auto f =
        reinterpret_cast<const char* (*)(int)>(symbol("VR_GetVRInitErrorAsEnglishDescription"));
    return f ? f(error) : "Arm Swing VR: original OpenVR library unavailable";
}
EXPORT const char* VR_GetVRInitErrorAsSymbol(int error) {
    const auto f = reinterpret_cast<const char* (*)(int)>(symbol("VR_GetVRInitErrorAsSymbol"));
    return f ? f(error) : "VRInitError_Init_InterfaceNotFound";
}
EXPORT const char* VR_GetStringForHmdError(int error) {
    return VR_GetVRInitErrorAsEnglishDescription(error);
}
EXPORT const char* VR_GetRuntimePathString() {
    const auto f = reinterpret_cast<const char* (*)()>(symbol("VR_GetRuntimePathString"));
    return f ? f() : "";
}
