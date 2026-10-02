<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Arm Swing VR

A Linux VR companion for arm-swing locomotion and controller mapping profiles,
written in C++20 with a Qt 6 Widgets interface. Target backends are SteamVR/OpenVR
and OpenXR, including games running through Proton.

**Status: project bootstrap / profile editor preview.** The editor creates,
opens, edits, and saves separate JSON files for each game's movement settings and
controller mappings. It does **not** yet read controller tracking, inject movement,
remap live inputs, automatically detect games, or install a VR driver. No game is
currently certified compatible. Input paths in profiles are draft configuration;
backend-specific validation will be added before they can be activated.

## Build

Requirements: a C++20 compiler, CMake 3.24+, Ninja, and Qt 6.4+ Core, Widgets,
and Test development packages. No VR runtime is required to build this preview.

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
./build/debug/arm-swing-vr
```

Use **File → New/Open/Save/Save as** to manage profiles. Save as can create a
variant for another game or controller. Activation input, measured arms, output
hand, and steering reference are independent settings. Unsaved changes prompt
before a profile is replaced or the application closes.

For AddressSanitizer and UndefinedBehaviorSanitizer checks with Clang:

```sh
cmake --preset asan
cmake --build --preset asan
ctest --preset asan
```

Qt widget tests run offscreen. A passing test suite verifies profile behavior;
it does not establish SteamVR or game compatibility. See [development and
integration testing](docs/DEVELOPMENT.md).

## Implementation direction

- A shared C++ motion processor with deterministic recording/replay tests.
- A Qt desktop application for per-game profiles, calibration, and diagnostics.
- A SteamVR driver/backend and a portable OpenXR API layer, tested separately.
- Explicit input ownership, normal-controller passthrough, and neutral output on
  stop, lost tracking, lost focus, stale data, or a profile change.
- OpenXR support for both two-dimensional actions and separate scalar X/Y actions.

The first hardware milestone is arm-swing movement plus an activation-button
mapping in Half-Life 2: VR Mod on Linux SteamVR. See the [roadmap](docs/ROADMAP.md).

## License and paid Steam distribution

Original project code, build files, profiles, and documentation are licensed under
**GNU GPL version 3 or later** (`GPL-3.0-or-later`), except where a file states
otherwise. See [LICENSE](LICENSE). Copyright © 2026 Arm Swing VR contributors.
Third-party components retain their own licenses and notices.

A paid Steam build is planned as a convenient way to install and update the same
GPL software. GPL permits charging for copies; recipients retain their rights to
study, modify, and redistribute them. Each distributed build must be accompanied
by access to its complete corresponding source under the GPL's terms.

The current project does not link the proprietary Steamworks SDK and grants no
Steamworks linking exception. OpenVR is a separate, publicly licensed SDK.
See the [licensing and Steam distribution policy](docs/LICENSING.md) and
[third-party dependency register](THIRD_PARTY.md).

This project is independent of Valve, Khronos, and Natural Locomotion. Product
names identify interoperability targets and do not imply affiliation.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). Issues and pull requests are welcome at
[johnny9/arm-swing-vr](https://github.com/johnny9/arm-swing-vr).
