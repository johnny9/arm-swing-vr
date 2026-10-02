#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "tools/openvr_game_adapter.py"

class InstallerTests(unittest.TestCase):
    def test_restore_and_preserve_updates(self):
        with tempfile.TemporaryDirectory(prefix="arm swing '") as directory:
            root = Path(directory)
            game = root / "openvr_api.dll"
            adapter = root / "adapter.dll"
            original = b"MZoriginal game library"
            game.write_bytes(original)
            adapter.write_bytes(b"MZadapter library")
            command = [sys.executable, str(SCRIPT)]
            result = subprocess.run(command + ["install", str(game), "--adapter", str(adapter), "--control", str(root / "ui.control")], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("ARMSWING_OPENVR_REAL", result.stdout)
            self.assertEqual(game.read_bytes(), adapter.read_bytes())
            again = subprocess.run(command + ["install", str(game), "--adapter", str(adapter), "--control", str(root / "ui.control")], capture_output=True)
            self.assertNotEqual(again.returncode, 0)
            game.write_bytes(b"MZgame update")
            refusal = subprocess.run(command + ["restore", str(game)], capture_output=True)
            self.assertNotEqual(refusal.returncode, 0)
            self.assertEqual(game.read_bytes(), b"MZgame update")
            self.assertEqual((root / "openvr_api.arm-swing-original.dll").read_bytes(), original)
            game.write_bytes(adapter.read_bytes())
            restored = subprocess.run(command + ["restore", str(game)], capture_output=True)
            self.assertEqual(restored.returncode, 0, restored.stderr)
            self.assertEqual(game.read_bytes(), original)
            self.assertFalse((root / "openvr_api.dll.arm-swing.json").exists())

unittest.main()
