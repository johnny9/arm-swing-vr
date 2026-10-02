// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "test_support.h"
#include <armswing/control.h>
#include <cmath>
#include <filesystem>
#include <limits>
using namespace armswing;
int main(int argc, char** argv) {
    CHECK(argc == 2);
    MotionProcessor processor;
    MotionConfig config;
    config.mappingCount = 1;
    config.mappings[0] = {0, 3};
    Sample sample;
    sample.focused = true;
    sample.head.valid = true;
    sample.hands[0].valid = sample.hands[1].valid = true;
    sample.buttons[0] = true;
    Output out;
    auto replay = [&](int rate, bool translate, bool swing) {
        processor.reset();
        for (int frame = 0; frame < rate; ++frame) {
            const double time = double(frame) / rate;
            sample.timeNs = 1000000000 + int64_t(time * 1e9);
            sample.head.position = {translate ? time : 0, 1.7, 0};
            sample.hands[0].position = {sample.head.position.x - .3,
                                        1 + (swing ? .15 * std::sin(time * 12) : 0), 0};
            sample.hands[1].position = {sample.head.position.x + .3,
                                        1 - (swing ? .15 * std::sin(time * 12) : 0), 0};
            out = processor.update(config, sample, true);
        }
        return out.movement.y;
    };
    const auto a = replay(90, false, true), b = replay(144, false, true);
    CHECK(a > .1 && a <= 1 && std::abs(a - b) < .07);
    const auto duplicate = processor.update(config, sample, true);
    CHECK(duplicate.movement.y == out.movement.y);
    CHECK(!out.buttons[0] && out.buttons[3]);
    CHECK(std::abs(replay(90, true, true) - a) < .0001);
    CHECK(replay(90, true, false) == 0);
    replay(90, false, true);
    sample.buttons[0] = false;
    sample.timeNs += 10000000;
    out = processor.update(config, sample, true);
    CHECK(out.movement.y == 0 && !out.buttons[3]);
    sample.buttons[0] = true;
    sample.focused = false;
    out = processor.update(config, sample, true);
    CHECK(!out.ownsInput && out.movement.y == 0);
    sample.focused = true;
    sample.hands[0].valid = false;
    CHECK(!processor.update(config, sample, true).ownsInput);
    sample.hands[0].valid = true;
    sample.head.position.x = std::numeric_limits<double>::quiet_NaN();
    CHECK(!processor.update(config, sample, true).ownsInput);
    sample.head.position.x = 0;
    CHECK(!processor.update(config, sample, false).ownsInput);
    config.steering = 1;
    sample.hands[0].forward = {1, 0};
    replay(90, false, true);
    CHECK(out.movement.x > .1 && std::abs(out.movement.y) < .001);
    CHECK(applyMovement({.7f, -.4f}, out).x == .7f);
    sample.timeNs += 1000000000;
    CHECK(processor.update(config, sample, true).movement.x == 0);
    config.mappings[0].destination = 0;
    CHECK(!validConfig(config));
    config.mappings[0] = {3, 0};
    CHECK(validConfig(config));
    sample.buttons[3] = true;
    const auto remapped = processor.update(config, sample, true);
    CHECK(remapped.buttons[0] && !remapped.buttons[3]);
    const std::string path = argv[1];
    setEnvironment("ARMSWING_CONTROL_FILE", path);
    ControlPacket p;
    p.enabled = 1;
    p.heartbeatNs = wallTimeNs();
    CHECK(writeControl(path, p));
    ControlPacket read;
    CHECK(readControl(Backend::OpenVr, read));
    CHECK(!readControl(Backend::OpenXr, read));
    p.heartbeatNs -= 1000000000;
    CHECK(writeControl(path, p));
    CHECK(!readControl(Backend::OpenVr, read));
    p.heartbeatNs = wallTimeNs();
    p.steamAppId = 123;
    CHECK(writeControl(path, p));
    setEnvironment("SteamAppId", "456");
    CHECK(!readControl(Backend::OpenVr, read));
    setEnvironment("SteamAppId", "123");
    CHECK(readControl(Backend::OpenVr, read));
    p.magic = 0;
    CHECK(writeControl(path, p));
    CHECK(!readControl(Backend::OpenVr, read));
    std::filesystem::remove(path);
    std::cout << "Motion replay, remapping, fail-safe and control lease checks passed\n";
}
