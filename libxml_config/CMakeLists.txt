
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared -DVERSION -Wl,--as-needed")

set(SOURCES_FILE
xml_config.cpp
)

set(LINK_LIBS
pthread
rt
dl
ssl
crypto
)

add_library(xml_config SHARED ${SOURCES_FILE})
SET_TARGET_PROPERTIES(xml_config PROPERTIES VERSION V.1.0 SOVERSION 0)
target_link_libraries(xml_config ${LINK_LIBS})
install(TARGETS xml_config DESTINATION bin)
