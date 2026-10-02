<!-- SPDX-License-Identifier: GPL-3.0-or-later -->
# Contributing

Contributions intentionally submitted for inclusion in original project files
must be available under `GPL-3.0-or-later`. Contributors retain their copyright;
no copyright assignment or contributor license agreement is required. Paid Steam
distribution of GPL builds is part of the project's intended distribution model.
This does not grant permission to close the source or override another author's
license.

Add an SPDX license identifier to new source/build files and preserve existing
copyright notices. Use a `.license` sidecar for formats such as JSON that do not
support comments. Original project documentation and profiles use the same GPL
license unless explicitly stated otherwise.

For imported code, record the upstream project, exact revision, original file,
license, copyright notices, and changes in THIRD_PARTY.md. Do not copy a
proprietary product's code, assets, or profile database. Discuss any new dependency
whose terms would change the distribution plan before integrating it. Preserve
per-file license terms even when the larger work is distributed under GPL.

Use C++20, Qt Quick/QML, Qt 6 public APIs, and the checked-in clang-format configuration.
Format QML with Qt's qmlformat. Keep shared visual tokens in qml/Theme.qml and
preserve its MIT attribution; new application components use the project GPL. Keep
motion processing independent of UI and runtime code. Keep runtime installations
and registration explicit; the build and test suite must not modify SteamVR,
Steam bindings, or OpenXR configuration.

Before submitting:

```sh
cmake --preset debug
cmake --build --preset debug
ctest --preset debug
```

Add meaningful regression coverage for changed behavior. For runtime changes,
record the game, runtime, controller, native/Proton path, test steps, and observed
result. Mark untested combinations as untested. Do not include account tokens,
personal Steam configuration, crash dumps, or device identifiers in public reports.
