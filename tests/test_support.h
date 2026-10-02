// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#define CHECK(condition)                                                                           \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::cerr << __FILE__ << ':' << __LINE__ << ": " #condition "\n";                      \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (false)
inline void setEnvironment(const char* name, const std::string& value) {
#ifdef _WIN32
    CHECK(_putenv_s(name, value.c_str()) == 0);
#else
    CHECK(setenv(name, value.c_str(), 1) == 0);
#endif
}
class Library {
    void* handle_;

  public:
    explicit Library(const char* path) {
#ifdef _WIN32
        handle_ = LoadLibraryA(path);
#else
        handle_ = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (!handle_)
            std::cerr << dlerror() << '\n';
#endif
        CHECK(handle_);
    }
    ~Library() {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
    }
    template <class T> T get(const char* name) const {
#ifdef _WIN32
        auto address = GetProcAddress(static_cast<HMODULE>(handle_), name);
#else
        auto address = dlsym(handle_, name);
#endif
        CHECK(address);
        T function;
        static_assert(sizeof(function) == sizeof(address));
        std::memcpy(&function, &address, sizeof(function));
        return function;
    }
};
