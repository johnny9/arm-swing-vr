// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../tests/test_support.h"
#include <chrono>
#include <openvr.h>
#include <thread>
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: openvr_probe ORIGINAL_API_LIBRARY [--start]\n";
        return 2;
    }
    setEnvironment("ARMSWING_OPENVR_REAL", argv[1]);
    setEnvironment("ARMSWING_CONTROL_FILE", "");
    std::cout << "Runtime installed: " << vr::VR_IsRuntimeInstalled()
              << "; HMD present: " << vr::VR_IsHmdPresent() << '\n';
    if (argc < 3 || std::string(argv[2]) != "--start")
        return 0;
    vr::EVRInitError error;
    auto* system = vr::VR_Init(&error, vr::VRApplication_Background);
    if (!system) {
        std::cerr << "OpenVR initialization: " << vr::VR_GetVRInitErrorAsEnglishDescription(error)
                  << '\n';
        return 1;
    }
    uint32_t width = 0, height = 0;
    system->GetRecommendedRenderTargetSize(&width, &height);
    std::cout << "OpenVR adapter initialized; render target " << width << 'x' << height << '\n';
    for (const char* version :
         {"IVRSystem_026", "IVRSystem_022", "IVRSystem_019", "IVRInput_011", "IVRInput_010"})
        std::cout << version << ": " << (vr::VR_GetGenericInterface(version, &error) != nullptr)
                  << '\n';
    for (int frame = 0; frame < 20; ++frame) {
        vr::TrackedDevicePose_t poses[vr::k_unMaxTrackedDeviceCount]{};
        system->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, poses,
                                                vr::k_unMaxTrackedDeviceCount);
        if (frame == 19) {
            for (const auto role :
                 {vr::TrackedControllerRole_LeftHand, vr::TrackedControllerRole_RightHand}) {
                const auto index = system->GetTrackedDeviceIndexForControllerRole(role);
                std::cout << (role == vr::TrackedControllerRole_LeftHand ? "Left" : "Right")
                          << " controller tracked: "
                          << (index < vr::k_unMaxTrackedDeviceCount && poses[index].bPoseIsValid)
                          << '\n';
            }
            std::cout << "HMD tracked: " << poses[0].bPoseIsValid << '\n';
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    vr::VR_Shutdown();
}
