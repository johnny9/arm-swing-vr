<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Third-party dependency register

Selected OpenAI Apps SDK UI design tokens are adapted in qml/Theme.qml under MIT;
their provenance and license are recorded below. Public VR API headers are
vendored; no third-party SDK binaries are vendored. The GPL text in LICENSE is the unmodified Free Software Foundation
license document, copied from the system's Bash license file; this is not an
import of Bash implementation code. The license document permits verbatim copying.

## Current build dependencies

| Component | Use | Selected terms / handling |
| --- | --- | --- |
| Qt 6 Core/GUI, QML/Quick, Quick Controls 2 and Quick Dialogs (Qt Base / Qt Declarative) | Dynamically linked native application/profile support and QML runtime | Use the LGPL-3.0 option where offered for these modules; retain Qt notices and comply with its source and relinking requirements when bundling it. Qt also offers GPL/commercial alternatives. |
| Qt 6 Test (Qt Base) | Development/test executables only | Same upstream licensing choices; not installed with the application. |
| C++ standard library | Compiler runtime | Toolchain-specific terms and exceptions; inventory the actual runtime before a binary release. |
| Python 3 | Build-time generation and optional setup/test scripts | Interpreter is not bundled. |
| Khronos OpenXR loader | Integration tests and runtime diagnostics | Dynamically loaded from the host; CI cross-builds the pinned SDK 1.1.63 loader for Wine tests. Apache-2.0 upstream terms, with upstream third-party notices. Not bundled with the application. |
| MinGW-w64 / GCC runtimes | Windows backend cross-builds | Static compiler runtimes in the local test DLLs; GCC Runtime Library Exception and component-specific MinGW notices/source must accompany a binary release. |

Qt includes components under additional licenses. A release must inventory the
specific Qt build and deployed plugins, not rely on this summary as their notices.
Qt is used through public APIs; no Qt Marketplace or proprietary
module is required.

Sources: [Qt licensing](https://doc.qt.io/qt-6/licensing.html),
[Qt LGPL obligations](https://www.qt.io/development/open-source-lgpl-obligations).

## Adapted UI design tokens

- Upstream: [openai/apps-sdk-ui](https://github.com/openai/apps-sdk-ui).
- Revision: `0f00143c7a639906f1621fe58e1b6be7b5bea46d`.
- Original files: `src/styles/variables-primitive.css`,
  `src/styles/variables-semantic.css`, `src/styles/variables-components.css`.
- Copyright: 2025 OpenAI. License: MIT, preserved verbatim at
  [third_party/apps-sdk-ui/LICENSE](third_party/apps-sdk-ui/LICENSE), installed with
  application license notices.
- Changes: selected neutral palette, surface/text roles, spacing and corner-radius
  values translated from CSS into QML properties in `qml/Theme.qml`; extra layout
  choices for a standalone desktop editor. The adapted theme file remains MIT.
- No React components, web runtime, fonts, logos, or proprietary desktop assets
  were imported. Other QML components and line icons are original project code
  under GPL-3.0-or-later. The combined application remains GPL-3.0-or-later.

## Vendored VR API declarations

All files below are unmodified upstream files, with licenses installed alongside
the backends. Valve declarations are compiled in version-specific namespaces so
different SDK generations do not conflict. Generated forwarding methods are
original project code; no runtime implementation or vtable patching is imported.

| Source | Exact revision | Files / terms |
| --- | --- | --- |
| [Valve OpenVR v2.15.6](https://github.com/ValveSoftware/openvr/tree/0924064316de3effbcd1acf1e309182a2deb1c05) | `0924064316de3effbcd1acf1e309182a2deb1c05` | `headers/openvr.h`, `headers/openvr_capi.h`, `LICENSE`; BSD-3-Clause, Valve Corporation. |
| [Valve OpenVR v2.0.10](https://github.com/ValveSoftware/openvr/tree/15f0838a0487feb7da60acd39aab8099b994234c) | `15f0838a0487feb7da60acd39aab8099b994234c` | `headers/openvr.h` saved as `legacy/openvr_2_0_10.h`; BSD-3-Clause. |
| [Valve OpenVR v1.0.17](https://github.com/ValveSoftware/openvr/tree/1fb1030f2ac238456dca7615a4408fb2bb42afb6) | `1fb1030f2ac238456dca7615a4408fb2bb42afb6` | `headers/openvr.h` saved as `legacy/openvr_1_0_17.h`; BSD-3-Clause. |
| [Khronos OpenXR SDK 1.1.63](https://github.com/KhronosGroup/OpenXR-SDK/tree/f2448a8797c85814aa892efc1ab8707900fbcc78) | `f2448a8797c85814aa892efc1ab8707900fbcc78` | `include/openxr/{openxr.h,openxr_platform_defines.h,openxr_loader_negotiation.h}`, `LICENSE`; Apache-2.0 option selected for dual-licensed headers. Copyright Khronos Group. |

Do not confuse the public OpenVR SDK with the proprietary Steamworks SDK.
No Steamworks SDK files are included or linked. Installed games' loader libraries
are used only as local interoperability test inputs and are never copied into
the repository or redistributed.

Existing locomotion projects have informed research only. No implementation or
profiles from Natural Locomotion, OpenVR-WalkInPlace, katwalk-linux, ArmSwinger,
OpenVR-InputEmulator, or OVR Advanced Settings have been copied. Their licenses
must be checked at the exact revision before any future reuse. In particular,
AGPL code would introduce obligations beyond this project's GPL baseline.
