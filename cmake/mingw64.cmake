# SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
# SPDX-License-Identifier: GPL-3.0-or-later
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)
find_program(ARMSWING_MINGW_CXX NAMES x86_64-w64-mingw32-g++-posix x86_64-w64-mingw32-g++ REQUIRED)
find_program(ARMSWING_MINGW_CC NAMES x86_64-w64-mingw32-gcc-posix x86_64-w64-mingw32-gcc REQUIRED)
set(CMAKE_CXX_COMPILER "${ARMSWING_MINGW_CXX}")
set(CMAKE_C_COMPILER "${ARMSWING_MINGW_CC}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(ARMSWING_BUILD_UI OFF CACHE BOOL "Desktop UI runs natively on Linux")
