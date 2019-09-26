
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared -DVERSION -Wl,--as-needed")

set(SOURCES_FILE
config_server.cpp
zk_config.cpp
dao_config.cpp
)

set(LINK_LIBS
    pthread
    rt
    dl
    ssl
    crypto
)

add_library(shmconfig_server SHARED ${SOURCES_FILE})
target_link_libraries(shmconfig_server ${LINK_LIBS})
install(TARGETS shmconfig_server DESTINATION bin)
