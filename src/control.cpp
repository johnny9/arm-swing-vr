// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <armswing/control.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <type_traits>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace armswing {
static_assert(std::is_trivially_copyable_v<ControlPacket>);
static_assert(sizeof(ControlPacket) == 96, "Update the protocol version when changing its layout");
int64_t wallTimeNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}
int64_t steadyTimeNs() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
static bool replace(const std::string& from, const std::string& to) {
#ifdef _WIN32
    return MoveFileExA(from.c_str(), to.c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    return std::rename(from.c_str(), to.c_str()) == 0;
#endif
}
bool writeControl(const std::string& path, const ControlPacket& packet) {
    const auto temporary = path + ".tmp";
    std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
    stream.write(reinterpret_cast<const char*>(&packet), sizeof(packet));
    stream.close();
#ifndef _WIN32
    chmod(temporary.c_str(), 0600);
#endif
    return stream && replace(temporary, path);
}
bool readControl(Backend backend, ControlPacket& out) {
    const char* path = std::getenv("ARMSWING_CONTROL_FILE");
    if (!path || !*path)
        return false;
    ControlPacket packet{};
    FILE* file = std::fopen(path, "rb");
    if (!file)
        return false;
#ifndef _WIN32
    struct stat st{};
    if (fstat(fileno(file), &st) != 0 || !S_ISREG(st.st_mode) || st.st_uid != getuid() ||
        (st.st_mode & 0077) != 0) {
        std::fclose(file);
        return false;
    }
#endif
    const bool complete =
        std::fread(&packet, 1, sizeof(packet), file) == sizeof(packet) && std::fgetc(file) == EOF;
    std::fclose(file);
    if (!complete || packet.magic != 0x41535652 || packet.version != 1 ||
        packet.backend != backend || packet.enabled != 1 || packet.heartbeatNs <= 0 ||
        !validConfig(packet.motion))
        return false;
    const auto age = wallTimeNs() - packet.heartbeatNs;
    if (age < -100000000 || age > 500000000)
        return false;
    if (packet.steamAppId) {
        const char* id = std::getenv("SteamAppId");
        if (!id || std::to_string(packet.steamAppId) != id)
            return false;
    }
    out = packet;
    return true;
}
int buttonIndex(const std::string& path) {
    for (int hand = 0; hand < 2; ++hand)
        for (int button = 0; button < 2; ++button)
            if (path == std::string("/user/hand/") + (hand ? "right" : "left") + "/input/" +
                            (button ? "b" : "a") + "/click")
                return hand * 2 + button;
    return -1;
}
Vec2 applyMovement(Vec2 physical, const Output& out) {
    if (!out.ownsInput || !std::isfinite(physical.x) || !std::isfinite(physical.y) ||
        std::hypot(physical.x, physical.y) > .2f)
        return physical;
    if (std::hypot(out.movement.x, out.movement.y) < .001f)
        return physical;
    return out.movement;
}
void reportStatus(Backend backend, const Output& out) {
    static thread_local int64_t previous = 0;
    const auto now = wallTimeNs();
    if (now - previous < 100000000 && now >= previous)
        return;
    previous = now;
    const char* path = std::getenv("ARMSWING_CONTROL_FILE");
    if (!path || !*path)
        return;
    const std::string destination = std::string(path) + ".status";
#ifdef _WIN32
    const auto pid = GetCurrentProcessId();
#else
    const auto pid = getpid();
#endif
    const auto temporary = destination + "." + std::to_string(pid) + ".tmp";
    std::ofstream file(temporary, std::ios::trunc);
    file << now << ' ' << uint32_t(backend) << ' ' << out.ownsInput << ' ' << out.movement.x << ' '
         << out.movement.y << '\n';
    file.close();
#ifndef _WIN32
    chmod(temporary.c_str(), 0600);
#endif
    if (file)
        replace(temporary, destination);
}
} // namespace armswing
