cmake_minimum_required(VERSION 3.22)

# Must be set before project(). macOS 11 is the oldest version that runs on both Intel and
# Apple Silicon, and is well below the macOS 13 requirement.
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

# Universal (x86_64 + arm64) builds take twice as long, so they are opt-in: CI turns this on,
# local development builds only for the machine you're on.
option(RAINR_UNIVERSAL "Build universal x86_64 + arm64 binaries on macOS" OFF)
if(APPLE AND RAINR_UNIVERSAL)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
elseif(APPLE AND CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64;arm64")
    set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "" FORCE) # back to this machine's architecture
endif()

project(RainrAudio LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# JUCE is a git submodule pinned to a release tag (see docs/CONVENTIONS.md).
add_subdirectory(JUCE)

# Code shared by every Rainr Audio plugin, as JUCE modules.
juce_add_module(shared/rainr_dsp)
juce_add_module(shared/rainr_ui)

# Brand fonts for rainr_ui (IBM Plex, SIL Open Font License: shared/rainr_ui/assets/fonts/OFL.txt).
file(GLOB RAINR_UI_FONTS CONFIGURE_DEPENDS shared/rainr_ui/assets/fonts/*.ttf)
juce_add_binary_data(RainrUiFonts HEADER_NAME RainrUiFonts.h NAMESPACE RainrUiFonts SOURCES ${RAINR_UI_FONTS})

include(RainrPlugin)

# One line per plugin.
add_subdirectory(plugins/WRM1)

# Engine tests against the Python reference (run with ctest).
enable_testing()
add_subdirectory(tests)

# Development helpers (not shipped).
add_subdirectory(tools)
