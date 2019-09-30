
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

set(SOURCES_FILE
shm_config.cpp
shm_config_go.cpp
)

add_library(shmconfig STATIC ${SOURCES_FILE})
install(TARGETS shmconfig DESTINATION bin)
