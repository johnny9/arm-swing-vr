// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "../tests/test_support.h"
#include <cstring>
#include <openxr/openxr.h>
int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: openxr_probe LOADER_LIBRARY LAYER_DIRECTORY\n";
        return 2;
    }
    setEnvironment("ARMSWING_CONTROL_FILE", "");
    setEnvironment("XR_API_LAYER_PATH", argv[2]);
    setEnvironment("XR_ENABLE_API_LAYERS", "XR_APILAYER_ARMSWING_locomotion");
    Library loader(argv[1]);
    const auto create = loader.get<PFN_xrCreateInstance>("xrCreateInstance");
    const auto get = loader.get<PFN_xrGetInstanceProcAddr>("xrGetInstanceProcAddr");
    XrInstanceCreateInfo info{};
    info.type = XR_TYPE_INSTANCE_CREATE_INFO;
    std::strcpy(info.applicationInfo.applicationName, "Arm Swing runtime probe");
    info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
    XrInstance instance;
    const auto result = create(&info, &instance);
    if (result != XR_SUCCESS) {
        std::cerr << "OpenXR instance creation returned " << result << '\n';
        return 1;
    }
    PFN_xrGetInstanceProperties propertiesFn = nullptr;
    PFN_xrDestroyInstance destroy = nullptr;
    PFN_xrGetSystem systemFn = nullptr;
    CHECK(get(instance, "xrGetInstanceProperties",
              reinterpret_cast<PFN_xrVoidFunction*>(&propertiesFn)) == XR_SUCCESS);
    CHECK(get(instance, "xrDestroyInstance", reinterpret_cast<PFN_xrVoidFunction*>(&destroy)) ==
          XR_SUCCESS);
    CHECK(get(instance, "xrGetSystem", reinterpret_cast<PFN_xrVoidFunction*>(&systemFn)) ==
          XR_SUCCESS);
    XrInstanceProperties properties{};
    properties.type = XR_TYPE_INSTANCE_PROPERTIES;
    CHECK(propertiesFn(instance, &properties) == XR_SUCCESS);
    std::cout << "OpenXR layer initialized with runtime: " << properties.runtimeName << '\n';
    XrSystemGetInfo systemInfo{};
    systemInfo.type = XR_TYPE_SYSTEM_GET_INFO;
    systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
    XrSystemId system;
    std::cout << "HMD system lookup result: " << systemFn(instance, &systemInfo, &system) << '\n';
    CHECK(destroy(instance) == XR_SUCCESS);
}
