
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static")

set(SOURCES_FILE
    src/go.cpp
    src/libconet.cpp
    src/schedule.cpp
    src/net_task.cpp
    src/net_clinet.cpp
    src/task_base.cpp
    src/log.cpp
    src/co_cond.cpp
    src/co_sem.cpp
    src/co_lock.cpp
    src/go_post.cpp
    src/task_queue.cpp
    src/context.cpp
    core/coroutine.cpp
    core/daemon.cpp
    core/event_epoll.cpp
    core/memory_pool.cpp
    core/socket_fd.cpp
    core/thread.cpp
    core/event_fd.cpp
    core/timer_fd.cpp
)

add_library(conet STATIC ${SOURCES_FILE})
install(TARGETS conet DESTINATION bin)
