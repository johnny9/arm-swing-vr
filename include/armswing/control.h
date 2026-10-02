// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <armswing/motion.h>
#include <string>

namespace armswing {
enum class Backend : uint32_t { OpenVr = 1, OpenXr = 2 };
// Local, versioned x86-64 protocol. Atomic file replacement prevents partial reads.
struct ControlPacket {
    uint32_t magic = 0x41535652;
    uint32_t version = 1;
    uint64_t generation = 0;
    int64_t heartbeatNs = 0;
    uint64_t steamAppId = 0;
    uint32_t enabled = 0;
    Backend backend = Backend::OpenVr;
    MotionConfig motion;
};
int64_t wallTimeNs();
int64_t steadyTimeNs();
bool readControl(Backend backend, ControlPacket& packet);
bool writeControl(const std::string& path, const ControlPacket& packet);
void reportStatus(Backend backend, const Output& output);
// Shared parser for the four supported digital controller paths.
int buttonIndex(const std::string& path);
Vec2 applyMovement(Vec2 physical, const Output& output);
} // namespace armswing
