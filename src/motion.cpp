// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <armswing/motion.h>
#include <cmath>

namespace armswing {
bool validConfig(const MotionConfig& c) {
    if (c.activation > 3 || c.arms > 2 || c.outputHand > 1 || c.steering > 2 ||
        !std::isfinite(c.sensitivity) || c.sensitivity < .1f || c.sensitivity > 5 ||
        c.mappingCount > c.mappings.size())
        return false;
    unsigned seen = 0;
    for (unsigned i = 0; i < c.mappingCount; ++i) {
        const auto m = c.mappings[i];
        if (m.source > 3 || m.destination > 3 ||
            (m.source == c.activation && m.destination == c.activation) ||
            (seen & (1u << m.source)))
            return false;
        seen |= 1u << m.source;
    }
    return true;
}
static bool validPose(const Pose& p) {
    return p.valid && std::isfinite(p.position.x) && std::isfinite(p.position.y) &&
           std::isfinite(p.position.z) && std::isfinite(p.forward.x) &&
           std::isfinite(p.forward.y) && std::hypot(p.forward.x, p.forward.y) > .01;
}
void MotionProcessor::reset() {
    havePrevious_ = false;
    speed_ = 0;
}
Output MotionProcessor::update(const MotionConfig& c, const Sample& s, bool enabled) {
    Output out;
    out.buttons = s.buttons;
    if (!enabled || !validConfig(c) || s.timeNs <= 0 || !s.focused || !validPose(s.head) ||
        !validPose(s.hands[c.activation / 2]) ||
        ((c.arms == 0 || c.arms == 2) && !validPose(s.hands[0])) ||
        ((c.arms == 1 || c.arms == 2) && !validPose(s.hands[1])) ||
        (c.steering && !validPose(s.hands[c.steering - 1]))) {
        reset();
        return out;
    }
    out.ownsInput = true;
    // Consume only the physical activation press. Another physical button may
    // still be remapped to its original game action.
    out.buttons[c.activation] = false;
    // Simultaneous remaps use the original snapshot, so cycles never cascade.
    for (unsigned i = 0; i < c.mappingCount; ++i)
        out.buttons[c.mappings[i].source] = false;
    for (unsigned i = 0; i < c.mappingCount; ++i)
        out.buttons[c.mappings[i].destination] =
            out.buttons[c.mappings[i].destination] || s.buttons[c.mappings[i].source];
    const double dt = (s.timeNs - previous_.timeNs) * 1e-9;
    const bool continuity = havePrevious_ && dt >= 0 && dt <= .1;
    const Sample previous = previous_;
    previous_ = s;
    havePrevious_ = true;
    if (!s.buttons[c.activation] || !continuity) {
        speed_ = 0;
        return out;
    }
    if (dt > 0) {
        double energy = 0;
        int count = 0;
        for (unsigned hand = 0; hand < 2; ++hand) {
            if (c.arms != 2 && hand != c.arms)
                continue;
            const auto& a = s.hands[hand].position;
            const auto& b = previous.hands[hand].position;
            // Remove head/body translation, then measure arm speed in metres/second.
            const double x = ((a.x - s.head.position.x) - (b.x - previous.head.position.x)) / dt;
            const double y = ((a.y - s.head.position.y) - (b.y - previous.head.position.y)) / dt;
            const double z = ((a.z - s.head.position.z) - (b.z - previous.head.position.z)) / dt;
            energy += x * x + y * y + z * z;
            ++count;
        }
        const double velocity = std::sqrt(energy / count);
        // Tracking jumps must not become maximum-speed movement.
        if (!std::isfinite(velocity) || velocity > 8) {
            reset();
            return out;
        }
        const double target = std::clamp((velocity - .15) * c.sensitivity / 1.5, 0.0, 1.0);
        speed_ += (target - speed_) * (1 - std::exp(-dt / .12));
    }
    const auto desired = c.steering ? s.hands[c.steering - 1].forward : s.head.forward;
    const auto reference = s.head.forward; // game must use head-relative smooth locomotion
    const double norm = std::hypot(desired.x, desired.y) * std::hypot(reference.x, reference.y);
    out.movement.x =
        float(speed_ * (double(desired.x) * -reference.y + double(desired.y) * reference.x) / norm);
    out.movement.y =
        float(speed_ * (double(desired.x) * reference.x + double(desired.y) * reference.y) / norm);
    return out;
}
} // namespace armswing
