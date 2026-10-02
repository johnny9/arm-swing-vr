<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Third-party dependency register

Selected OpenAI Apps SDK UI design tokens are adapted in qml/Theme.qml under MIT;
their provenance and license are recorded below. No third-party SDK binaries are
vendored. The GPL text in LICENSE is the unmodified Free Software Foundation
license document, copied from the system's Bash license file; this is not an
import of Bash implementation code. The license document permits verbatim copying.

## Current build dependencies

| Component | Use | Selected terms / handling |
| --- | --- | --- |
| Qt 6 Core/GUI, QML/Quick, Quick Controls 2 and Quick Dialogs (Qt Base / Qt Declarative) | Dynamically linked native application/profile support and QML runtime | Use the LGPL-3.0 option where offered for these modules; retain Qt notices and comply with its source and relinking requirements when bundling it. Qt also offers GPL/commercial alternatives. |
| Qt 6 Test (Qt Base) | Development/test executables only | Same upstream licensing choices; not installed with the application. |
| C++ standard library | Compiler runtime | Toolchain-specific terms and exceptions; inventory the actual runtime before a binary release. |

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

## Planned dependencies — not linked or bundled yet

| Component | Upstream baseline | Planned use |
| --- | --- | --- |
| [Valve OpenVR SDK](https://github.com/ValveSoftware/openvr/blob/master/LICENSE) | BSD-3-Clause | SteamVR integration; preserve Valve's license and attribution. |
| [Khronos OpenXR SDK](https://github.com/KhronosGroup/OpenXR-SDK/blob/main/LICENSE) | Apache-2.0, subject to per-file terms | OpenXR API layer and loader integration; preserve required licenses/notices. |

Pin exact SDK revisions and record the licenses of the files actually imported
when adding these dependencies. Do not confuse Valve's OpenVR SDK with its
proprietary Steamworks SDK. No Steamworks SDK files are included or linked.

Existing locomotion projects have informed research only. No implementation or
profiles from Natural Locomotion, OpenVR-WalkInPlace, katwalk-linux, ArmSwinger,
OpenVR-InputEmulator, or OVR Advanced Settings have been copied. Their licenses
must be checked at the exact revision before any future reuse. In particular,
AGPL code would introduce obligations beyond this project's GPL baseline.
