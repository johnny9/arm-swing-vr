// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "fixture.h"
#ifdef _WIN32
#define EXPORT extern "C" __declspec(dllexport)
#else
#define EXPORT extern "C"
#endif
VrFixtureFrame vrFixture;
void* fixtureCurrent(const char*);
void* fixture22(const char*);
void* fixture19(const char*);
EXPORT void fixtureSetFrame(const VrFixtureFrame* f) { vrFixture = *f; }
EXPORT void* VR_GetGenericInterface(const char* name, int* error) {
    if (error)
        *error = 0;
    if (auto* p = fixtureCurrent(name))
        return p;
    if (auto* p = fixture22(name))
        return p;
    if (auto* p = fixture19(name))
        return p;
    if (error)
        *error = 105;
    return nullptr;
}
EXPORT unsigned VR_InitInternal(int* error, int) {
    if (error)
        *error = 0;
    return 1;
}
EXPORT unsigned VR_InitInternal2(int* error, int type, const char*) {
    return VR_InitInternal(error, type);
}
EXPORT void VR_ShutdownInternal() {}
EXPORT bool VR_IsRuntimeInstalled() { return true; }
EXPORT bool VR_IsHmdPresent() { return true; }
EXPORT unsigned VR_GetInitToken() { return 1; }
