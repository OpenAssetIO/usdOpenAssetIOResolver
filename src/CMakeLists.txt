
set(PLUGIN_NAME usdOpenAssetIOResolver)

set(
  SRC
    resolver.cpp
)

add_library(${PLUGIN_NAME}
    SHARED
    ${SRC}
)

target_link_libraries(${PLUGIN_NAME}
    PUBLIC
    ar
)

#-----------------------------------------------------------------------
# Activate warnings as errors, pedantic, etc.
set_default_compiler_warnings(${PLUGIN_NAME})


#-----------------------------------------------------------------------
# Add static analysis dependencies
if (OPENASSETIO_USDRESOLVER_ENABLE_CLANG_FORMAT)
    add_dependencies(${PLUGIN_NAME} openassetio-clangformat)
endif ()
if (OPENASSETIO_USDRESOLVER_ENABLE_CPPLINT)
    add_dependencies(${PLUGIN_NAME} openassetio-cpplint)
endif ()

#-----------------------------------------------------------------------
# Main install
install(
    TARGETS
        ${PLUGIN_NAME}
    DESTINATION
        .
)

#-----------------------------------------------------------------------
# Install plugInfo.json
install(
    FILES
		resources/plugInfo.json
    DESTINATION
        ./resources
)
