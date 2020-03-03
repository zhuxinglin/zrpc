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

#include "coroutine.h"
#include "co_lock.h"
#include "thread.h"
#include "task_base.h"
#include "uconfig.h"
#include "task_queue.h"
#include "go_post.h"
#include "context.h"

using namespace znet;

extern CContext* g_pContext;

CCoLock::CCoLock() : m_wLock(0),m_wSync(0)
{
}

CCoLock::~CCoLock()
{
}

void CCoLock::Lock()
{
    ITaskBase* pTask = g_pContext->m_pCo->GetTaskBase();
    while (__sync_lock_test_and_set(&m_wLock, 1))
    {
        {
            CSpinLock<uint16_t> oLock(m_wSync);
            m_oLock.push(pTask->m_qwCid);
        }
        pTask->Yield(-1, ITaskBase::RUN_LOCK);
    }
}

void CCoLock::Unlock()
{
    CContext* pCx = g_pContext;
    uint64_t qwCoId = Pop();
    __sync_lock_release(&m_wLock);

    ITaskBase* pTask = pCx->m_pCo->GetTaskBase();

    if (qwCoId != 0 && qwCoId != pTask->m_qwCid)
    {
        pCx->m_pTaskQueue->SwapWaitToExec(qwCoId);
        CGoPost::Post();
    }
}

uint64_t CCoLock::Pop()
{
    if (m_oLock.empty())
        return 0;

    uint64_t qwCoId = m_oLock.front();
    CSpinLock<uint16_t> oLock(m_wSync);
    m_oLock.pop();
    return qwCoId;
}

