# SPDX-License-Identifier: Apache-2.0
# Copyright 2013-2022 The Foundry Visionmongers Ltd

include(CompilerWarnings)

function(openassetio_usdresolver_set_default_target_properties target_name)
    #-------------------------------------------------------------------
    # C++ standard

    # Minimum C++ standard as per current VFX reference platform CY21+.
    target_compile_features(${target_name} PRIVATE cxx_std_17)

    set_target_properties(
        ${target_name}
        PROPERTIES
        # Ensure the proposed compiler supports our minimum C++
        # standard.
        CXX_STANDARD_REQUIRED ON
        # Disable compiler extensions. E.g. use -std=c++11  instead of
        # -std=gnu++11.  Helps limit cross-platform issues.
        CXX_EXTENSIONS OFF
    )

    #-------------------------------------------------------------------
    # Compiler warnings
    openassetio_usdresolver_set_default_compiler_warnings(${target_name})

    #-------------------------------------------------------------------
    # Symbol visibility

    # Hide symbols from this library by default.
    set_target_properties(
        ${target_name}
        PROPERTIES
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
    )

    # Hide all symbols from external statically linked libraries.
    if (IS_GCC_OR_CLANG AND NOT APPLE)
        # TODO(TC): Find a way to hide symbols on macOS
        target_link_options(${target_name} PRIVATE "-Wl,--exclude-libs,ALL")
    endif ()

    # Whether to use the old or new C++ ABI with gcc.
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
        CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 5.0)
        if (OPENASSETIO_USDRESOLVER_GLIBCXX_USE_CXX11_ABI)
            target_compile_definitions(${target_name} PRIVATE _GLIBCXX_USE_CXX11_ABI=1)
        else ()
            target_compile_definitions(${target_name} PRIVATE _GLIBCXX_USE_CXX11_ABI=0)
        endif ()
    endif ()

    #-------------------------------------------------------------------
    # Library version

    get_target_property(target_type ${target_name} TYPE)

    if (NOT ${target_type} STREQUAL EXECUTABLE)
        # When building or installing appropriate symlinks are created, if
        # supported.
        set_target_properties(
            ${target_name}
            PROPERTIES
            VERSION ${PROJECT_VERSION}
            SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        )
    endif ()

    #-------------------------------------------------------------------
    # Linters/analyzers

    if (TARGET openassetio-usdresolver-cpplint)
        add_dependencies(${target_name} openassetio-usdresolver-cpplint)
    endif ()

    if (TARGET openassetio-usdresolver-clangformat)
        add_dependencies(${target_name} openassetio-usdresolver-clangformat)
    endif ()

endfunction()