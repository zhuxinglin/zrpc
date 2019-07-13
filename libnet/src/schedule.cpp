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


#include "schedule.h"
#include "net_task.h"
#include "task_queue.h"
#include "coroutine.h"
#include "timer_fd.h"
#include "go_post.h"

using namespace znet;

CSchedule* CSchedule::m_pSelf = 0;

CSchedule::CSchedule()
{
}

CSchedule::~CSchedule()
{
    Exit();
}

int CSchedule::Initialize(void *pUserData)
{
    int iRet = m_oEvent.Create();
    if (iRet < 0)
    {
        return iRet;
    }
    return 0;
}

int CSchedule::PushMsg(uint32_t dwId, uint32_t dwMsgType, int iMsgLen, void *pMsg)
{
    return m_oEvent.SetCtl(dwId, dwMsgType, iMsgLen, pMsg);
}

std::string CSchedule::GetErr()
{
    return m_oEvent.GetErr();
}

CSchedule *CSchedule::GetObj()
{
    if (!m_pSelf)
        m_pSelf = new CSchedule();
    return m_pSelf;
}

void CSchedule::Release()
{
    if (m_pSelf)
        delete m_pSelf;
    m_pSelf = 0;
}

void CSchedule::Run(uint32_t dwId)
{
    struct epoll_event ev[256];
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    uint64_t qwLastTime = CTimerFd::GetUs();
    while (m_bExit)
    {
        int iCount = m_oEvent.Wait(ev, 256, 1);
        if (iCount < 0 || !m_bExit)
            break;

        int iRet;
        for (int i = 0; i < iCount; i++)
        {
            iRet = pTaskQueue->SwapWaitToExec(ev[i].data.u64, false);
            if (iRet == 0)
                CGoPost::Post();
        }

        uint64_t qwCurTime = CTimerFd::GetUs();
        uint32_t dwTimeout = qwCurTime - qwLastTime;
        if (dwTimeout < 1e6)
            continue;
		
		qwLastTime = qwCurTime;

        iRet = pTaskQueue->SwapTimerToExec(qwCurTime);
        if (iRet == 0)
            CGoPost::Post();
    }
}

