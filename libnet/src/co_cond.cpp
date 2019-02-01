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

using namespace znet;

CCoCond::CCoCond()
{
}

CCoCond::~CCoCond()
{
}

void CCoCond::Signal()
{
    uint64_t qwCoId = m_oCond.front();
    m_oCond.pop();
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    pTaskQueue->SwapWaitToExec(qwCoId);
    CGoPost::Post();
}

void CCoCond::Broadcast()
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    while (m_oCond.empty())
    {
        uint64_t qwCoId = m_oCond.front();
        m_oCond.pop();
        pTaskQueue->SwapWaitToExec(qwCoId);
        CGoPost::Post();
    }
}

bool CCoCond::Wait(CCoLock *pLock, uint32_t dwTimeout)
{
    if (!pLock)
        return false;

    ITaskBase *pTask = CCoroutine::GetObj()->GetTaskBase();
    m_oCond.push(pTask->m_qwCid);
    pLock->Unlock();

    bool bRet = true;
    if (pTask->Yield(dwTimeout) < 0)
        bRet = false;

    pLock->Lock();
    return bRet;
}


