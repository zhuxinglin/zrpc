
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

set(SOURCES_FILE
    src/zk_api.cpp
    src/zk_proto_mgr.cpp
    src/watcher_event.cpp
)

add_library(zk STATIC ${SOURCES_FILE})
install(TARGETS zk DESTINATION bin)
