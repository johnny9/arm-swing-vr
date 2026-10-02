// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
struct XrFixtureFrame {
    float swing = 0;
    float manualX = 0, manualY = 0;
    bool activation = true;
    bool focused = true;
    bool tracked = true;
    bool actionActive = true;
};
struct XrFixtureStats {
    unsigned attachedSets = 0, syncedSets = 0, suggestions = 0;
};
