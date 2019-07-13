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

using namespace znet;

CCoSem::CCoSem()
{
}

CCoSem::~CCoSem()
{
}

void CCoSem::Post()
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    uint64_t qwCid;
    {
        CSpinLock oLock(m_dwLock);
        m_dwCount ++;
        if (m_oQueue.empty())
            return;

        auto it = m_oQueue.begin();
        qwCid = *it;
        m_oQueue.erase(it);
    }
    pTaskQueue->SwapWaitToExec(qwCid);
    CGoPost::Post();
}

bool CCoSem::Wait(uint32_t dwTimeout)
{
    ITaskBase *pTask = CCoroutine::GetObj()->GetTaskBase();
    {
        CSpinLock oLock(m_dwLock);
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
            CSpinLock oLock(m_dwLock);
            m_oQueue.erase(pTask->m_qwCid);
            return false;
        }

        {
            CSpinLock oLock(m_dwLock);
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
    CSpinLock oLock(m_dwLock);
    if (m_dwCount != 0)
        return true;
    return false;
}
