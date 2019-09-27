
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared -DVERSION -Wl,--as-needed")

set(SOURCES_FILE
config_server.cpp
dao_config.cpp
)

set(LINK_LIBS
    pthread
    rt
    dl
    ssl
    crypto
)

add_library(xmlconfig_server SHARED ${SOURCES_FILE})
SET_TARGET_PROPERTIES(xmlconfig_server PROPERTIES VERSION V.1.0 SOVERSION 0)
target_link_libraries(xmlconfig_server ${LINK_LIBS})
install(TARGETS xmlconfig_server DESTINATION bin)
