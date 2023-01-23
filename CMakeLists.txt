# 1. Requirements
project(usdOpenAssetIOResolver)
cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#-----------------------------------------------------------------------
# Default install directory

# Default install to a `dist` directory under the build directory, ready
# for use in tests and for packaging. But don't override if user has
# explicitly set CMAKE_INSTALL_PREFIX.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND PROJECT_IS_TOP_LEVEL)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/dist" CACHE PATH "Installation location" FORCE)
endif ()

add_compile_options(-Wno-deprecated)  # This is just to avoid some spammy warnings

# Find USD.
# This searches for the `pxrConfig.cmake` file that USD provides, using
# cmakes config search mode. Normally the USD directory will be on the
# system path as per the install, so it will be found.
find_package(pxr REQUIRED)

add_subdirectory(src)
