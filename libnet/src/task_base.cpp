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
#include "context.h"
#include "go_post.h"

using namespace znet;

extern CContext* g_pContext;

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
                         m_wRunStatus(RUN_INIT),
                         m_wRunStatusLock(0),
                         m_wExitMode(AUTO_EXIT)
{
}

ITaskBase::~ITaskBase()
{
    if (m_pSp && m_pContext)
    {
        g_pContext->m_pCo->Del(this);
        m_pSp = 0;
        m_pContext = 0;
    }

    if (m_pTaskQueue)
    {
        g_pContext->m_pTaskQueue->DelWaitTask((CTaskNode*)m_pTaskQueue);
        m_pTaskQueue = 0;
    }
}

int ITaskBase::YieldEventDel(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent)
{
    return g_pContext->m_pCo->GetTaskBase()->Yield(dwTimeoutMs, iFd, YIELD_ADD, YIELD_DEL, iSetEvent, iRestoreEvent, RUN_WAIT);
}

int ITaskBase::YieldEventRestore(uint32_t dwTimeoutMs, int iFd, int iSetEvent, int iRestoreEvent)
{
    return g_pContext->m_pCo->GetTaskBase()->Yield(dwTimeoutMs, iFd, YIELD_MOD, YIELD_MOD, iSetEvent, iRestoreEvent, RUN_WAIT);
}

int ITaskBase::Yield(uint32_t dwTimeoutMs, uint8_t wRunStatus)
{
    return g_pContext->m_pCo->GetTaskBase()->Yield(dwTimeoutMs, -1, 0, 0, 0, 0, wRunStatus);
}

int ITaskBase::Sleep(uint32_t dwTimeoutMs)
{
    return g_pContext->m_pCo->GetTaskBase()->Yield(dwTimeoutMs, -1, 0, 0, 0, 0, RUN_SLEEP);
}

int ITaskBase::Yield(uint32_t dwTimeoutMs, int iFd, int iSetOpt, int iRestoreOpt, int iSetEvent, int iRestoreEvent, uint8_t wRunStatus)
{
    uint32_t dwTimeout = m_dwTimeout;
    if (dwTimeoutMs != 0xFFFFFFFF)
        m_dwTimeout = dwTimeoutMs * 1e3;
    else
        m_dwTimeout = dwTimeoutMs;

    while (__sync_lock_test_and_set(&m_wRunStatusLock, 1));
    m_wRunStatus = (RUN_EXIT & m_wRunStatus);
    if (wRunStatus != RUN_WAIT)
        m_wRunStatus |= wRunStatus;
    else
        m_wRunStatus |= RUN_WAIT;
    __sync_lock_release(&m_wRunStatusLock);

    if (m_dwTimeout != dwTimeout)
    {
        // 重新设置超时
        g_pContext->m_pTaskQueue->UpdateTimeout(m_qwCid);
    }

    if (iSetEvent != -1 && iFd != -1)
        g_pContext->m_pSchedule->PushMsg(iFd, iSetOpt, iSetEvent, (void *)m_qwCid);

    m_wStatus = STATUS_TIME;
    g_pContext->m_pCo->Swap(this, false);

    if (iRestoreEvent != -1 && iFd != -1)
        g_pContext->m_pSchedule->PushMsg(iFd, iRestoreOpt, iRestoreEvent, (void *)m_qwCid);

    if (m_dwTimeout != dwTimeout)
    {
        m_dwTimeout = dwTimeout;
        // 重新设置超时
        g_pContext->m_pTaskQueue->UpdateTimeout(m_qwCid);
    }

    if (m_wStatus == STATUS_TIMEOUT)
        return -2;

    if (m_wRunStatus & RUN_EXIT)
        return -1;

    return 0;
}

uint64_t ITaskBase::GenCid(int iFd)
{
    uint32_t dwCid = __sync_fetch_and_add(&g_pContext->m_dwCidInc, 1);

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

bool ITaskBase::IsExitCo() const
{
    return m_wRunStatus & RUN_EXIT;
}

void ITaskBase::SetManualModeExit(uint8_t dwExitMode)
{
    m_wExitMode = dwExitMode;
}

void ITaskBase::CloseCo()
{
    if (m_wExitMode == MANUAL_EXIT_SO)
    {
        g_pContext->m_pTaskQueue->AddExecTask((CTaskNode*)m_pTaskQueue, false);
        CGoPost::Post();
        return ;
    }
}
