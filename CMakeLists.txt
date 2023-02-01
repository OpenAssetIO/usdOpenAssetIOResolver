# 1. Requirements
cmake_minimum_required(VERSION 3.21)

project(
usdOpenAssetIOResolver
VERSION 1.0.0
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Additional include directories for CMake utils.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)

#-----------------------------------------------------------------------
# Default install directory

# Default install to a `dist` directory under the build directory, ready
# for use in tests and for packaging. But don't override if user has
# explicitly set CMAKE_INSTALL_PREFIX.
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT AND PROJECT_IS_TOP_LEVEL)
    set(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/dist" CACHE PATH "Installation location" FORCE)
endif ()

#-----------------------------------------------------------------------
# Lint options

# Default treating compiler warnings as errors to OFF, since
# consumers of this project may use unpredictable toolchains.
# For dev/CI we should remember to switch this ON, though!
option(OPENASSETIO_USDRESOLVER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

# Enable clang-format formatting check.
option(OPENASSETIO_USDRESOLVER_ENABLE_CLANG_FORMAT "Enable clang-format check during build" OFF)

# Enable clang-tidy static analysis tool.
option(OPENASSETIO_USDRESOLVER_ENABLE_CLANG_TIDY "Enable clang-tidy analysis during build" OFF)

# Enable cpplint linter.
option(OPENASSETIO_USDRESOLVER_ENABLE_CPPLINT "Enable cpplint linter during build" OFF)


# Disable C++11 ABI by default, as per current VFX reference
# platform(s), but allow optionally enabling.
# For reference, note that the default varies by platform, e.g.
# * CentOS 7.9.2009's GCC 6.3, `_GLIBCXX_USE_CXX11_ABI` is
#   always `0` and cannot be overridden.
# * Ubuntu 18.04's GCC 6.5, `_GLIBCXX_USE_CXX11_ABI` defaults
#   to `1`, but can be overridden.
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.0)
    option(OPENASSETIO_USDRESOLVER_GLIBCXX_USE_CXX11_ABI "For gcc, use the new C++11 library ABI" OFF)
endif ()


# Find USD.
find_package(pxr REQUIRED)

# Find OpenAssetIO
find_package(OpenAssetIO REQUIRED)

# Add Static analysis targets
include(StaticAnalyzers)
if (OPENASSETIO_USDRESOLVER_ENABLE_CLANG_FORMAT)
    enable_clang_format()
endif ()
if (OPENASSETIO_USDRESOLVER_ENABLE_CLANG_TIDY)
    enable_clang_tidy()
endif ()
if (OPENASSETIO_USDRESOLVER_ENABLE_CPPLINT)
    enable_cpplint()
endif ()

include(DefaultTargetProperties)
add_subdirectory(src)

#-----------------------------------------------------------------------
# Print a status dump
message(STATUS "Warnings as errors              = ${OPENASSETIO_USDRESOLVER_WARNINGS_AS_ERRORS}")
message(STATUS "Linter: clang-tidy              = ${OPENASSETIO_USDRESOLVER_ENABLE_CLANG_TIDY} [${OPENASSETIO_USDRESOLVER_CLANGTIDY_EXE}]")
message(STATUS "Linter: cpplint                 = ${OPENASSETIO_USDRESOLVER_ENABLE_CPPLINT} [${OPENASSETIO_USDRESOLVER_CPPLINT_EXE}]")
message(STATUS "Linter: clang-format            = ${OPENASSETIO_USDRESOLVER_ENABLE_CLANG_FORMAT} [${OPENASSETIO_USDRESOLVER_CLANGFORMAT_EXE}]")
