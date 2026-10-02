// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <array>
#include <cstdint>

namespace armswing {
struct Vec2 {
    float x = 0, y = 0;
};
struct Vec3 {
    double x = 0, y = 0, z = 0;
};
struct Pose {
    Vec3 position;
    // Horizontal forward direction in the common tracking space, -Z at identity.
    Vec2 forward{0, -1};
    bool valid = false;
};
struct Mapping {
    uint32_t source = 0, destination = 0;
};
struct MotionConfig {
    uint32_t activation = 0; // left A, left B, right A, right B
    uint32_t arms = 2;       // left, right, both
    uint32_t outputHand = 0;
    uint32_t steering = 0; // head, left, right
    float sensitivity = 1;
    uint32_t mappingCount = 0;
    std::array<Mapping, 4> mappings{};
};
struct Sample {
    int64_t timeNs = 0;
    bool focused = false;
    Pose head;
    std::array<Pose, 2> hands{};
    std::array<bool, 4> buttons{};
    std::array<Vec2, 2> sticks{};
};
struct Output {
    bool ownsInput = false;
    Vec2 movement;
    std::array<bool, 4> buttons{};
};
bool validConfig(const MotionConfig& config);
class MotionProcessor {
  public:
    Output update(const MotionConfig& config, const Sample& sample, bool enabled);
    void reset();

  private:
    Sample previous_;
    bool havePrevious_ = false;
    double speed_ = 0;
};
} // namespace armswing
