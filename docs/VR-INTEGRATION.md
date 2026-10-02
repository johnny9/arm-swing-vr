<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# VR integration prototype

The backends run in the selected game's process. OpenVR uses an API adapter;
OpenXR uses a loader-discovered API layer. A virtual treadmill driver cannot by
itself rewrite another controller's inputs, so this prototype uses the API
boundary instead. No driver registration or system-wide runtime replacement is
needed. These are experimental integrations, not game compatibility certifications.

## Implemented behavior

- Arm speed is measured from changes in controller positions relative to the
  head, with a dead zone, time-based smoothing, bounded output, and rejection of
  tracking jumps. Synthetic replay verifies frame-rate independence and removal
  of common head/body translation. Real motion calibration remains necessary.
- Holding the chosen Index A/B input enables movement. The physical activation
  press is consumed. Remaps are simultaneous: for example, physical right B can
  supply the original left A action while left A is reserved for locomotion.
- Head/left/right steering is converted to **head-relative** joystick axes.
  Configure the game's smooth-locomotion reference accordingly. Manual stick
  magnitude above 0.2 takes precedence. Sprint/teleport modes are not implemented.
- Editing, switching, or stopping a profile removes its control lease. Closing
  or losing the editor expires it after 500 ms. The backend returns to physical
  input at its next input sample/sync; synthetic values never persist in hardware.
- Lost focus, invalid tracking, missing activation bindings, and pose gaps over
  100 ms stop generated movement. Unrelated rendering, haptics, and input calls
  are forwarded. A Steam App ID restricts activation to a matching `SteamAppId`
  environment variable; leave it blank for explicitly launched diagnostic/non-Steam apps.

## Supported API surface and limits

OpenVR supports System **019, 022, 026** and Input **010, 011**, through both C++
interfaces and C function tables. Forwarders are generated from the exact Valve
headers, not guessed vtable offsets. Other interface versions pass through.
Legacy joystick axes are discovered from device properties, and controller
packet numbers advance when synthetic state changes.

Action-based OpenVR games must already expose the activation and mapping source
as active, simple click actions and request their handles. The adapter reads
these original actions, since legacy controller polling may be unavailable once
an action manifest is selected. Ambiguous multi-input bindings, toggle/chord
modes, unbound inputs, and button-event-only consumers are outside current support.
Keep the activation button bound in SteamVR; the adapter consumes its delivered
click. Multiple active action sets and custom bindings still require game testing.

OpenXR currently adds private Index pose/A/B/stick actions to the application's
action-set attachment and synchronizes them while enabled. It merges its Index
bindings with the game's suggestions, queries actual bound sources, and handles
vector2 actions and separate float X/Y actions. Per-hand queries are filtered by
subaction path. Results and change timestamps are stable between syncs. Action,
action-set, session, and instance cleanup release cached state. Unsupported
bindings/actions pass through. Controller profiles beyond Valve Index are pending.

The desktop editor validates the supported live-input subset when enabling a
profile. Saving a draft can still preserve future/custom input paths. Backend
status proves that the adapter is running and sampling; it does not by itself
prove that a particular game is using a rewritten locomotion action.

## Native Linux setup

Build normally and open `build/debug/arm-swing-vr`. Select or create a profile,
use **Launch setup**, then enable it and launch the game. The dialog provides
environment options scoped to that game. OpenXR needs the built layer manifest
directory in `XR_API_LAYER_PATH` and its name in `XR_ENABLE_API_LAYERS`.

OpenVR `LD_PRELOAD` works for applications that resolve API exports globally.
Games that explicitly load their own OpenVR library can bypass it. Use the
reversible installer for those games, with the game closed:

```sh
python3 tools/openvr_game_adapter.py install /path/to/game/libopenvr_api.so \
  --adapter build/debug/libarmswing_openvr.so \
  --control '/path/from/the/editor/ui.ini.control'
```

The helper preserves the original library and prints exact Steam launch options,
including `ARMSWING_OPENVR_REAL`. It refuses a second install over an existing
backup. Restore with:

```sh
python3 tools/openvr_game_adapter.py restore /path/to/game/libopenvr_api.so
```

Restore verifies the original backup and refuses to overwrite a game update or
another modification. Remove the Arm Swing launch options afterward. No real
game library is changed by the test suite. Linux builds must also match the
game's Steam Linux Runtime/libstdc++ ABI; the host build is not a portable release.

## Windows/Proton setup

Build x86-64 DLLs with MinGW-w64; the Qt UI continues running natively:

```sh
cmake -S . -B build/windows -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=cmake/mingw64.cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build build/windows
```

For OpenVR, run the same installer against the game's `openvr_api.dll`, using
`--adapter build/windows/openvr_api.dll`. It prints launch options with Wine's
native DLL override and `Z:` paths to the original library and editor control
file. The adapter's standard-library runtime is statically linked. Its backup
and restore checks are the same as native Linux. Use the game's actual DLL
architecture; this prototype does not build 32-bit adapters.

For OpenXR, use `build/windows/openxr/armswing.json` and enable
`XR_APILAYER_ARMSWING_locomotion` for the game. Windows loaders such as the copy
bundled with Metro Awakening discover explicit layers through the selected
Proton prefix's `HKLM\Software\Khronos\OpenXR\1\ApiLayers\Explicit` registry key:
the manifest's absolute `Z:` path is the value name, `REG_DWORD`, value `0`.
Register only that explicit layer in the chosen prefix; remove the value to
unregister it. **Do not replace that game's ActiveRuntime entry with the test
fixture.** Keep its normal SteamVR runtime. Launch with `ARMSWING_CONTROL_FILE`
using the editor file's `Z:` path and `XR_ENABLE_API_LAYERS` set to the layer name.
Other Wine drive mappings must be translated accordingly.

Steam/Proton container loading and game behavior still require per-game
verification. Passing Wine fixture tests alone does not prove those environments.

## Automated integration tests

```sh
ctest --preset debug
ctest --preset debug -L integration
ctest --preset asan
python3 tools/test_wine.py \
  --wine /path/to/proton/files/bin/wine64 \
  --build build/windows --prefix build/armswing-test-prefix \
  --openxr-loader /path/to/installed/game/openxr_loader.dll
```

The Wine runner only accepts an empty prefix or one it previously marked as a
test prefix. It registers a fixture runtime **inside that test prefix only**.
No existing game prefix is used. Without `--openxr-loader`, it explicitly skips
the Windows OpenXR test. CI builds the public Khronos loader from the pinned
OpenXR-SDK 1.1.63 revision and runs both Windows backends under Wine; no game
installation is required. Its workflow also documents the cross-build commands.

The tests exercise the **compiled plugin binaries**, not alternate mock backend
implementations. Controlled downstream fixtures supply deterministic tracking
and button values. OpenXR passes through a real Khronos loader. Coverage includes
movement, source suppression/destination activation, independent hands, vector2
and scalar axes, manual stick priority, repeated/omitted queries, release,
tracking/focus loss, stale leases, passthrough, and lifecycle cleanup. Separate
tests cover motion replay, invalid profiles, UI enable/stop, editor ownership,
and restoring an installation without overwriting a game update.

## Local evidence — 2026-10-02

| Check | Evidence / boundary |
| --- | --- |
| Linux OpenVR | System 019/022/026 C++ and flat-table clients tested against the loadable adapter; action tests disable legacy polling. |
| Linux OpenXR | Host Khronos loader 1.1.63 loads the layer and fixture runtime; vector2/scalar/remapping checks pass. |
| Windows OpenVR | Same six interface/client combinations passed under Proton 10.0's Wine 10.0 in a dedicated prefix. |
| Windows OpenXR | Passed with both a source-built Khronos loader 1.1.63 and Metro Awakening's installed Windows loader, using the controlled runtime in that test prefix. The game itself was not launched. |
| Installed hardware | OpenVR reports runtime installed and HMD present. Background initialization returns error 121 while SteamVR is stopped; no tracked-pose or game result is claimed. |
| Game ABI inspection | Installed Half-Life 2 VR client requests System 026/Input 011; native Talos VR requests System 019. This identifies targets, not compatibility. |

The remaining acceptance check is a user in the headset: actual movement and a
working activation remap in Half-Life 2 VR, preserved grabbing/aiming/menu controls,
manual-stick precedence, release/stop, and recovery after focus/tracking loss.
Repeat separately in native Talos VR and OpenXR Metro Awakening. Record game,
runtime, Proton version, settings, and observed results before marking a game
compatible.

References: [Valve API declarations](https://github.com/ValveSoftware/openvr),
[OpenXR loader design](https://registry.khronos.org/OpenXR/specs/1.1/loader.html),
[OpenXR action synchronization](https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#input).
