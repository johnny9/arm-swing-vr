<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Roadmap

## 0. Repository and profile foundation

- [x] C++20, Qt Quick/QML, CMake/Ninja, CTest, and Clang sanitizer presets.
- [x] Native dark/light theme, profile sidebar, recent files, duplicate, and QML interaction tests.
- [x] Editable per-game profile files containing mapping and arm-swing settings.
- [x] GPL-3.0-or-later licensing, dependency register, and paid-distribution policy.
- [x] GitHub CI configuration for GCC and Clang sanitizer builds.

## 1. First playable SteamVR prototype

- [x] Integrate pose/button acquisition at the OpenVR/OpenXR API boundary.
- [x] Shared motion estimator with synthetic replay tests; hardware trace collection remains pending.
- [x] Prove movement and button remapping in diagnostic clients using controlled runtime fixtures.
- [x] Explicit Enable/Stop and live backend status; edits stop input.
- [x] Handle stale samples, focus/tracking loss, and stop adding input after lease expiry.
- [ ] Automatic game discovery and calibration.
- [ ] Validate in Half-Life 2: VR Mod with the headset and controllers.

## 2. Additional games and OpenXR

- [ ] Validate native OpenVR in The Talos Principle VR and interaction in H3VR.
- [x] OpenXR layer, including float/vector2 axes and subaction paths.
- [x] Windows backend fixture tests under Proton Wine, including Metro's installed OpenXR loader.
- [ ] Validate Proton/OpenXR in Metro Awakening.
- [ ] Add per-game locomotion calibration, steering-frame conversion, sprint,
      activation passthrough policies, and controller/runtime profile variants.
- [ ] Publish a versioned compatibility matrix backed by test evidence.

## 3. Distribution

- [ ] Reversible runtime registration/uninstall and dependency-compatible Linux builds.
- [ ] Bundle complete corresponding source, licenses, and third-party notices.
- [ ] Test clean installation, update, rollback, and removal.
- [ ] Prepare an optional paid Steam package from the same GPL source.

This roadmap carries no release-date or game-compatibility commitment.
