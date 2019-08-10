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

#include "co_cond.h"
#include "coroutine.h"
#include "go_post.h"
#include "task_queue.h"
#include "thread.h"
#include "context.h"

using namespace znet;

extern CContext* g_pContext;

CCoCond::CCoCond()
{
}

CCoCond::~CCoCond()
{
}

void CCoCond::Signal()
{
    uint64_t qwCoId;
    {
        CSpinLock oLock(m_dwLock);
        if (m_oCond.empty())
            return;

        qwCoId = m_oCond.front();
        m_oCond.pop();
    }

    g_pContext->m_pTaskQueue->SwapWaitToExec(qwCoId);
    CGoPost::Post();
}

void CCoCond::Broadcast()
{
    while (m_oCond.empty())
    {
        uint64_t qwCoId;
        {
            CSpinLock oLock(m_dwLock);
            qwCoId = m_oCond.front();
            m_oCond.pop();
        }

        g_pContext->m_pTaskQueue->SwapWaitToExec(qwCoId);
        CGoPost::Post();
    }
}

bool CCoCond::Wait(CCoLock *pLock, uint32_t dwTimeout)
{
    if (!pLock)
        return false;

    ITaskBase *pTask = g_pContext->m_pCo->GetTaskBase();
    m_oCond.push(pTask->m_qwCid);
    pLock->Unlock();

    bool bRet = true;
    if (pTask->Yield(dwTimeout, ITaskBase::RUN_LOCK) < 0)
        bRet = false;

    pLock->Lock();
    return bRet;
}


