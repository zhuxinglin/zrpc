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

#include "task_base.h"
#include "task_queue.h"
#include "coroutine.h"
#include "schedule.h"
#include <stdio.h>
#include "timer_fd.h"
#include "event_epoll.h"

volatile uint32_t g_dwCid = 0;
typedef union _Cid
{
    struct 
    {
        uint32_t dwCid;
        int iFd;
    }U_s;
    uint64_t qwCid;
}CCid;

ITaskBase::ITaskBase() : m_pSp(0),
                         m_pContext(0),
                         m_pMainCo(0),
                         m_pNewObj(0),
                         m_pTaskQueue(0),
                         m_qwCid(0),
                         m_dwTimeout(0),
                         m_wStatus(0),
                         m_wProtocol(0),
                         m_wIsRuning(true),
                         m_wRunStatus(RUN_INIT)
{
}

int ITaskBase::YieldEventDel(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent)
{
    return Yield(dwTimeoutMs, iFd, YIELD_ADD, YIELD_DEL, iSetEvent, iRestoreEvent);
}

int ITaskBase::YieldEventRestore(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent)
{
    return Yield(dwTimeoutMs, iFd, YIELD_MOD, YIELD_MOD, iSetEvent, iRestoreEvent);
}

int ITaskBase::Yield(uint32_t dwTimeoutMs)
{
    return Yield(dwTimeoutMs, -1, 0, 0, 0, 0);
}

int ITaskBase::Yield(uint32_t dwTimeoutMs, int iFd, int iSetOpt, int iRestoreOpt, int iSetEvent, int iRestoreEvent)
{
    CCoroutine *pCor = CCoroutine::GetObj();

    m_wStatus = STATUS_TIME;
    m_wRunStatus = RUN_WAIT;
    uint32_t dwTimeout = m_dwTimeout;
    if (dwTimeoutMs || m_wProtocol == PROTOCOL_TIMER)
    {
        if (dwTimeoutMs != (uint32_t)-1)
            m_dwTimeout = dwTimeoutMs * 1e3;
    }

    CThread *pSch = CSchedule::GetObj();

    if (iSetEvent != -1 && iFd != -1)
        pSch->PushMsg(iFd, iSetOpt, iSetEvent, (void *)m_qwCid);

    pCor->Swap(this, false);

    if (iRestoreEvent != -1 && iFd != -1)
        pSch->PushMsg(iFd, iRestoreOpt, iRestoreEvent, (void *)m_qwCid);
    m_dwTimeout = dwTimeout;

    if (m_wStatus == STATUS_TIMEOUT)
        return -1;
    return 0;
}

uint64_t ITaskBase::GenCid(int iFd)
{
    uint32_t dwCid = __sync_fetch_and_add(&g_dwCid, 1);
    __sync_val_compare_and_swap(&g_dwCid, 0x7FFFFFFF, 0);

    CCid oCid;
    oCid.U_s.dwCid = dwCid;
    oCid.U_s.iFd = iFd;
    return oCid.qwCid;
}

int ITaskBase::GetFd(uint64_t qwCid)
{
    CCid oCid;
    oCid.qwCid = qwCid;
    return oCid.U_s.iFd;
}

uint32_t ITaskBase::GetSubCId(uint64_t qwCid)
{
    CCid oCid;
    oCid.qwCid = qwCid;
    return oCid.U_s.dwCid;
}
