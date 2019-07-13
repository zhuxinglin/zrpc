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

using namespace znet;

typedef union _Cid
{
    struct 
    {
        uint32_t dwCid;
        int iFd;
    }U_s;
    uint64_t qwCid;
}CCid;

ITaskBase::ITaskBase() : m_oPtr(this),
                         m_pSp(0),
                         m_pContext(0),
                         m_pMainCo(0),
                         m_pCb(0),
                         m_pTaskQueue(0),
                         m_pData(0),
                         m_qwConnectTime(0),
                         m_qwCid(0),
                         m_dwTimeout(0),
                         m_wStatus(0),
                         m_wProtocol(0),
                         m_wIsRuning(true),
                         m_wRunStatus(RUN_INIT)
{
}

ITaskBase::~ITaskBase()
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    CCoroutine *pCor = CCoroutine::GetObj();

    if (m_pSp && m_pContext)
    {
        pCor->Del(this);
        m_pSp = 0;
        m_pContext = 0;
    }

    if (m_pTaskQueue)
    {
        pTaskQueue->DelWaitTask((CTaskNode*)m_pTaskQueue);
        m_pTaskQueue = 0;
    }
}

int ITaskBase::YieldEventDel(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent)
{
    return Yield(dwTimeoutMs, iFd, YIELD_ADD, YIELD_DEL, iSetEvent, iRestoreEvent, RUN_WAIT);
}

int ITaskBase::YieldEventRestore(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent)
{
    return Yield(dwTimeoutMs, iFd, YIELD_MOD, YIELD_MOD, iSetEvent, iRestoreEvent, RUN_WAIT);
}

int ITaskBase::Yield(uint32_t dwTimeoutMs, uint8_t wRunStatus)
{
    return Yield(dwTimeoutMs, -1, 0, 0, 0, 0, wRunStatus);
}

int ITaskBase::Yield(uint32_t dwTimeoutMs, int iFd, int iSetOpt, int iRestoreOpt, int iSetEvent, int iRestoreEvent, uint8_t wRunStatus)
{
    CCoroutine *pCor = CCoroutine::GetObj();

    m_wStatus = STATUS_TIME;

    uint32_t dwTimeout = m_dwTimeout;
    if (dwTimeoutMs != 0xFFFFFFFF)
        m_dwTimeout = dwTimeoutMs * 1e3;
    else
        m_dwTimeout = dwTimeoutMs;

    if (dwTimeout != m_dwTimeout)
    {
        // 重新设置超时
        CTaskQueue::GetObj()->UpdateTimeout(m_qwCid);
    }

    if (wRunStatus != RUN_WAIT)
        m_wRunStatus = RUN_LOCK;
    else
        m_wRunStatus = RUN_WAIT;

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
    static volatile uint32_t dwCidInc = 0;
    uint32_t dwCid = __sync_fetch_and_add(&dwCidInc, 1);

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
