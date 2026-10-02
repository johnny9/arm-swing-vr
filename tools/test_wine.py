#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the built Windows plugins against fixtures in an isolated Wine prefix."""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--wine", required=True, type=Path)
parser.add_argument("--build", default="build/windows", type=Path)
parser.add_argument("--prefix", required=True, type=Path)
parser.add_argument("--openxr-loader", type=Path,
                    help="A built or installed 64-bit Windows Khronos loader DLL (loaded in place)")
args = parser.parse_args()
build = args.build.resolve()
prefix = args.prefix.resolve()
marker = prefix / ".arm-swing-test-prefix"
if prefix.exists() and any(prefix.iterdir()) and not marker.exists():
    parser.error("Refusing an existing prefix not created by this test runner. Choose an empty directory.")
prefix.mkdir(parents=True, exist_ok=True)
marker.write_text("Dedicated Arm Swing VR integration test prefix.\n")
env = dict(os.environ, WINEPREFIX=str(prefix), WINEDEBUG="-all", WINEDLLOVERRIDES="winemenubuilder.exe=d")
env.pop("ARMSWING_CONTROL_FILE", None)
env.pop("SteamAppId", None)
def win(path):
    return "Z:" + str(Path(path).resolve())
def run(exe, *arguments):
    print(f"Testing {exe} {' '.join(map(str, arguments))}", flush=True)
    subprocess.run([str(args.wine), str(build / exe), *map(str, arguments)], env=env, check=True, timeout=90)
for version in ("Current", "22", "19"):
    for abi in ("cpp", "flat"):
        run(f"openvr_integration_{version}.exe", win(build / "openvr_api.dll"),
            win(build / "libfixture_openvr.dll"), win(build / f"wine-{version}-{abi}.control"), abi)
def registry(key, name, kind, data):
    subprocess.run([str(args.wine), "reg", "add", key, "/v", name, "/t", kind, "/d", data, "/f"],
                   env=env, check=True, timeout=90)
# Older Windows loaders discover runtimes and layers only through the registry.
# These writes affect only the dedicated test prefix, never a Steam game prefix.
if args.openxr_loader:
    registry(r"HKLM\Software\Khronos\OpenXR\1", "ActiveRuntime", "REG_SZ", win(build / "fixtures/openxr-runtime.json"))
    registry(r"HKLM\Software\Khronos\OpenXR\1\ApiLayers\Explicit", win(build / "openxr/armswing.json"), "REG_DWORD", "0")
    run("openxr_integration.exe", win(args.openxr_loader), win(build / "libfixture_openxr.dll"),
        win(build / "fixtures/openxr-runtime.json"), win(build / "openxr"),
        win(build / "wine-openxr.control"), win(build / "armswing_openxr.dll"))
    print("All Windows backend fixture integrations passed under Wine.")
else:
    print("Windows OpenVR integrations passed. OpenXR Windows test skipped: no loader DLL supplied.")
