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

#include "co_sem.h"
#include "coroutine.h"
#include "go_post.h"
#include "task_queue.h"
#include "context.h"

using namespace znet;

extern CContext* g_pContext;

CCoSem::CCoSem()
{
}

CCoSem::~CCoSem()
{
}

void CCoSem::Post()
{
    uint64_t qwCid;
    {
        CSpinLock<> oLock(m_dwLock);
        m_dwCount ++;
        if (m_oQueue.empty())
            return;

        auto it = m_oQueue.begin();
        qwCid = *it;
        m_oQueue.erase(it);
    }
    g_pContext->m_pTaskQueue->SwapWaitToExec(qwCid);
    CGoPost::Post();
}

bool CCoSem::Wait(uint32_t dwTimeout)
{
    ITaskBase *pTask = g_pContext->m_pCo->GetTaskBase();
    {
        CSpinLock<> oLock(m_dwLock);
        if (m_dwCount != 0)
        {
            --m_dwCount;
            return true;
        }
        m_oQueue.insert(pTask->m_qwCid);
    }

    while (1)
    {
        if (pTask->Yield(dwTimeout, ITaskBase::RUN_LOCK) < 0)
        {
            CSpinLock<> oLock(m_dwLock);
            m_oQueue.erase(pTask->m_qwCid);
            return false;
        }

        {
            CSpinLock<> oLock(m_dwLock);
            if (m_dwCount == 0)
                continue;

            --m_dwCount;
        }
        break;
    }
    return true;
}

bool CCoSem::TryWait()
{
    CSpinLock<> oLock(m_dwLock);
    if (m_dwCount != 0)
        return true;
    return false;
}
