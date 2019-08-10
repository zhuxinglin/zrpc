/*
*
*
*
*
*
*
*
*
*
*
*
*
*
*/

#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include <coroutine.h>
#include <atomic>
#include "go.h"
#include "thread.h"
#include "schedule.h"
#include "task_queue.h"
#include <string>
#include "memory_pool.h"
#include "event_epoll.h"

namespace znet
{

class CContext
{
public:
    CContext();
    ~CContext();

    int Init(uint32_t dwWorkThreadCount);

public:
    CCoroutine* m_pCo;
    CGo *m_pGo;
    CTaskQueue* m_pTaskQueue;
    CSchedule* m_pSchedule;
    std::atomic_uint m_dwExecTheadCount{0};
    uint32_t m_dwWorkThreadCount;
    uint32_t m_dwMaxTaskCount{1000000};
    CSem m_oSem;
    CEventEpoll m_oEvent;
    CMemoryPool m_oNetPool;
    std::string m_sErr;
    volatile uint32_t m_dwCidInc{0};
};


}

#endif
