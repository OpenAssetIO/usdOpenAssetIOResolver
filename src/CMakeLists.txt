
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

install(
    TARGETS
        ${PLUGIN_NAME}
    DESTINATION
        .
)

install(
    FILES
		resources/plugInfo.json
    DESTINATION
        ./resources
)
