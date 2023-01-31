
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
    PRIVATE
    ar
    OpenAssetIO::openassetio-core
    OpenAssetIO::openassetio-python-bridge
)

#-----------------------------------------------------------------------
# Activate warnings as errors, pedantic, etc.
openassetio_usdresolver_set_default_target_properties(${PLUGIN_NAME})

# ----------------------------------------------------------------------
# Silence warnings about deprecation from USD itself
# At time of writing, from : from USD/include/pxr/base/tf/hashset.h:39
target_compile_options(${PLUGIN_NAME} PRIVATE -Wno-deprecated)

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
