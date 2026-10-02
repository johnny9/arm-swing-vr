#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Install/restore a per-game OpenVR adapter with an integrity-checked backup.

The build and tests never call this against a real game. Close the game first.
Use install explicitly; restore refuses to overwrite a game update or other mod.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import tempfile

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def replace(source, destination):
    descriptor, temporary = tempfile.mkstemp(prefix=".arm-swing-", dir=destination.parent)
    os.close(descriptor)
    try:
        shutil.copy2(source, temporary)
        os.replace(temporary, destination)
    finally:
        Path(temporary).unlink(missing_ok=True)

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("operation", choices=("install", "restore"))
    parser.add_argument("game_library", type=Path)
    parser.add_argument("--adapter", type=Path)
    parser.add_argument("--control", type=Path)
    args = parser.parse_args()
    game = args.game_library.resolve(strict=True)
    if game.name != "openvr_api.dll" and not game.name.startswith("libopenvr_api.so"):
        parser.error("Choose the game's openvr_api.dll or libopenvr_api.so library.")
    backup = game.with_name(game.stem + ".arm-swing-original" + game.suffix)
    record = game.with_name(game.name + ".arm-swing.json")
    if args.operation == "restore":
        state = json.loads(record.read_text())
        if state["backup"] != str(backup) or digest(backup) != state["original_sha256"]:
            parser.error("The original backup does not match the install record; preserving all files.")
        if digest(game) not in (state["installed_sha256"], state["original_sha256"]):
            parser.error("The game library changed since installation; preserving the update and backup.")
        replace(backup, game)
        backup.unlink()
        record.unlink()
        print("Original game library restored. Remove Arm Swing VR's Steam launch options.")
        return
    if not args.adapter or not args.control:
        parser.error("install requires --adapter and --control (the editor's control file).")
    adapter = args.adapter.resolve(strict=True)
    if backup.exists() or record.exists() or game == adapter:
        parser.error("An installation/backup already exists, or the adapter is the game library. Restore first.")
    magic = b"MZ" if game.suffix == ".dll" else b"\x7fELF"
    if not game.read_bytes().startswith(magic) or not adapter.read_bytes().startswith(magic):
        parser.error("The adapter and game library must have the same native/Windows format.")
    shutil.copy2(game, backup)
    state = {"backup": str(backup), "original_sha256": digest(backup), "installed_sha256": digest(adapter)}
    # Record the expected transition first, allowing restore after an interrupted copy.
    with record.open("x") as stream:
        json.dump(state, stream, indent=2)
        stream.write("\n")
    replace(adapter, game)
    control = str(args.control.resolve())
    original = str(backup)
    override = ""
    if magic == b"MZ":
        control, original = "Z:" + control, "Z:" + original
        override = 'WINEDLLOVERRIDES="openvr_api=n,b${WINEDLLOVERRIDES:+;$WINEDLLOVERRIDES}" '
    print("Installed. Steam launch options for this game:")
    print(override + "ARMSWING_CONTROL_FILE=" + shlex.quote(control) +
          " ARMSWING_OPENVR_REAL=" + shlex.quote(original) + " %command%")

if __name__ == "__main__":
    main()
