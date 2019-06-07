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

using namespace znet;

CCoLock::CCoLock() : m_dwLock(0)
{
}

CCoLock::~CCoLock()
{
}

void CCoLock::Lock()
{
    while (__sync_lock_test_and_set(&m_dwLock, 1))
        Push();
}

void CCoLock::Unlock()
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();

    uint64_t qwCoId = Pop();
    __sync_lock_release(&m_dwLock);

    ITaskBase* pTask = CCoroutine::GetObj()->GetTaskBase();

    if (qwCoId != 0 && qwCoId != pTask->m_qwCid)
    {
        pTaskQueue->SwapWaitToExec(qwCoId);
        CGoPost::Post();
    }
}

void CCoLock::Push()
{
    ITaskBase* pTask = CCoroutine::GetObj()->GetTaskBase();
    pTask->m_wRunStatus = ITaskBase::RUN_LOCK;
    uint64_t qwCoId = pTask->m_qwCid;
    m_oLock.push(qwCoId);

    pTask->Yield(-1);
}

uint64_t CCoLock::Pop()
{
    if (m_oLock.empty())
        return 0;

    uint64_t qwCoId = m_oLock.front();
    m_oLock.pop();
    return qwCoId;
}

