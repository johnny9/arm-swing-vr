// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
struct VrFixtureFrame {
    float swing = 0;
    float manualX = 0, manualY = 0;
    bool activation = true;
    bool focused = true;
    bool tracked = true;
    bool actionActive = true;
    bool failRead = false;
    bool unsupportedBinding = false;
    bool legacyDisabled = false;
};
extern VrFixtureFrame vrFixture;
