<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Development and integration testing

## Current checks

The CMake/CTest presets build the C++/QML profile editor and test serialization,
validation, per-game separation, preservation of existing files on invalid saves,
profile switching, and independent input/output-hand selection. QML tests use
real keyboard/mouse input, exercise unsaved-change dialogs, and render dark/light
themes and compact layouts. Controller tests cover mapping edits without model
resets, duplication, failed saves, and appearance/recent-file persistence. These tests need
no headset. The Clang sanitizer preset enables ASan and UBSan.

To regenerate visual QA captures without changing real user profiles:

```sh
ARMSWING_TEST_SCREENSHOTS=/tmp/arm-swing-vr-previews ctest --preset debug -R qml_ui
```

The QML tests use software rendering on the offscreen platform and temporary
settings files. Also test the native graphics backend on the target desktop
before a release. QML resources are embedded; the application needs no browser,
Node.js, npm package, or OpenAI service at runtime.

The current development machine has Qt 6.11.2, GCC, Clang, CMake, Ninja, GDB, and
the OpenXR development files. GCC sanitizer runtimes were missing at initial
inspection; Clang's sanitizer runtime compiled and ran successfully. No system
package installation is required for the current scaffold.

Use a normal host session for real GPU/device/runtime tests. A restricted build
sandbox can compile successfully while lacking access required for SteamVR.
Keep host capability failures separate from application defects.

LeakSanitizer also needs process-inspection access that some sandboxes prohibit.
If it reports that it cannot operate under ptrace, run the same tests in a normal
host session. Do not disable leak checking in CI to hide that environment issue.

## Runtime work still required

1. Pin and audit the Valve/Khronos SDK revisions before integrating them.
2. Implement a runtime-independent C++ arm-motion processor. Replay timestamped
   pose recordings to test start/stop, frame-rate independence, jitter, head/body
   movement compensation, and accidental arm motion during normal interactions.
3. Build small OpenVR/OpenXR diagnostic applications that report the input values
   actually delivered. Cover OpenVR legacy input and SteamVR actions; cover both
   vector2 and scalar float OpenXR axes, subaction paths, and action-set focus.
4. Verify driver/layer loading in native Linux and the real Proton/Steam runtime
   environment. A host build may need a Steam Linux Runtime SDK build to satisfy
   library ABI requirements inside its containers.
5. Verify input suppression/remapping as well as added movement. A virtual
   treadmill controller alone does not prove control over physical-controller
   inputs. Preserve ordinary game controls and make input ownership explicit.
6. Add explicit, reversible per-user registration and uninstall tooling. Record
   and restore configuration that the tool changes. Builds/tests must not install
   runtime plugins or replace game libraries automatically.

Useful tools: GDB, SteamVR logs, Proton logs, SteamVR input bindings, and Khronos
[API dump/core validation layers](https://github.com/KhronosGroup/OpenXR-SDK-Source/blob/main/src/api_layers/README.md).
Record timing for pose capture, motion estimation, and delivered input. Do not
publish personal runtime logs without reviewing and redacting them.

## Initial hardware test matrix

These targets were identified from local installed binaries and bindings. They
are proposed tests, **not compatibility claims**. Use the game's appropriate
smooth-locomotion settings and record the exact runtime/Proton version.

| Game | Intended coverage | Status |
| --- | --- | --- |
| Half-Life 2: VR Mod | Proton/OpenVR; explicit movement, sprint, and jump actions | Untested |
| The Talos Principle VR | Native Linux/OpenVR | Untested |
| Metro Awakening | Proton/OpenXR; separate left-thumbstick X/Y float actions | Untested |
| H3VR | OpenVR; movement alongside weapon handling and interaction | Untested |
| Beat Saber | No synthetic input when no locomotion profile is active | Untested |

First prove a diagnostic application's observed input, then repeat in the game.
For each game record activation/release, low/high swing speed, steering while
looking elsewhere, normal grabbing/aiming, manual stick precedence, menus/focus,
tracking loss, profile changes, process loss, restart, and restoration after exit.
User-in-headset testing is required to assess movement feel and real interaction.

Acceptance of the first playable prototype requires actual movement and a working
activation remap in the selected game, neutral output when disabled, and retained
normal controls. Passing desktop tests or seeing a virtual device is insufficient.
