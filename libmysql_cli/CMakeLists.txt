
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

set(SOURCES_FILE
src/mysql_cli.cpp
src/mysql_helper.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/../code/include)

add_library(mysql_cli STATIC ${SOURCES_FILE})
install(TARGETS mysql_cli DESTINATION bin)
