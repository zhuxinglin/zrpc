
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--as-needed -D_REENTRANT -Wno-invalid-")

set(SOURCES_FILE
    main.cpp
    src/config.cpp
    src/http_parser.cpp
    src/http_svc.cpp
    src/monitor_so.cpp
    src/corpc.cpp
    src/so_plugin.cpp
    src/so_uninstall.cpp
    src/binary_svc.cpp
    src/binary_handle.cpp
)

set(LINK_LIBS
    pthread
    rt
    dl
    conet
    ssl
    crypto
)

add_executable(corpc ${SOURCES_FILE})
target_link_libraries(corpc ${LINK_LIBS})
install(TARGETS corpc DESTINATION bin)
