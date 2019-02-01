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

CCoSem::CCoSem() : m_qwCurId(0)
{
}

CCoSem::~CCoSem()
{
}

void CCoSem::Post()
{
    if (m_qwCurId == 0)
        return;
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    pTaskQueue->SwapWaitToExec(m_qwCurId);
    CGoPost::Post();
}

bool CCoSem::Wait(uint32_t dwTimeout)
{
    ITaskBase *pTask = CCoroutine::GetObj()->GetTaskBase();
    m_qwCurId = pTask->m_qwCid;
    if (pTask->Yield(dwTimeout) < 0)
        return false;
    return true;
}
