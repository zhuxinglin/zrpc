
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared -DVERSION -Wl,--as-needed")

set(SOURCES_FILE
config_agent.cpp
config_zk.cpp
config_watcher.cpp
config_shm.cpp
)

set(LINK_LIBS
    pthread
    rt
    dl
    ssl
    crypto
)

add_library(config_agent SHARED ${SOURCES_FILE})
SET_TARGET_PROPERTIES(config_agent PROPERTIES VERSION V.1.0 SOVERSION 0)
target_link_libraries(config_agent ${LINK_LIBS})
install(TARGETS config_agent DESTINATION bin)
