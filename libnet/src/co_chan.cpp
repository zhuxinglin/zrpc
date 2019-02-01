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

#include "co_chan.h"
#include "task_queue.h"
#include "go_post.h"
#include "thread.h"

using namespace znet;

CCoChan::CCoChan(ITaskBase *pTask) : m_pTask(pTask)
{
    m_qwCoId = pTask->m_qwCid;
}

CCoChan::~CCoChan()
{
}

bool CCoChan::Write(void *pMsg, uint32_t dwType)
{
    CChan oChan;
    oChan.dwType = dwType;
    oChan.pMsg = pMsg;

    {
        CSpinLock oLock(m_dwSync);
        m_oChan.push(oChan);
    }
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    pTaskQueue->SwapWaitToExec(m_qwCoId);
    CGoPost::Post();
    return true;
}

uint32_t CCoChan::operator()(uint32_t dwTimeout)
{
    if (m_pTask->Yield(dwTimeout) < 0)
        return -1;

    CSpinLock oLock(m_dwSync);
    const CChan& oChan = m_oChan.front();
    return oChan.dwType;
}

void *CCoChan::GetMsg()
{
    CSpinLock oLock(m_dwSync);
    CChan oChan = m_oChan.front();
    m_oChan.pop();
    return oChan.pMsg;
}

