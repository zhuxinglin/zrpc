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
#include "go.h"

CSchedule* CSchedule::m_pSelf = 0;
extern CGo *g_pGo;
extern uint32_t g_dwWorkThreadCount;

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
    uint64_t dwLastTime = CTimerFd::GetUs();
    CGo *pGo = g_pGo;
    uint32_t dwIndex = 0;
    while (m_bExit)
    {
        int iCount = m_oEvent.Wait(ev, 256, 10);
        if (iCount < 0 || !m_bExit)
            break;

        int iRet;
        for (int i = 0; i < iCount; i++)
        {
            iRet = pTaskQueue->SwapWaitExec(ev[i].data.u64);
            if (iRet == 0)
                pGo[dwIndex ++].PushMsg(0, 0, 0, 0);
            if (dwIndex >= g_dwWorkThreadCount)
                dwIndex = 0;
        }

        uint64_t dwCurTime = CTimerFd::GetUs();
        uint32_t dwTimeout = dwCurTime - dwLastTime;
        if (dwTimeout < 10 * 1e6)
            continue;

        iRet = pTaskQueue->SwapTimerExec(dwCurTime);
        if (iRet == 0)
            pGo[dwIndex ++].PushMsg(0, 0, 0, 0);
        if (dwIndex >= g_dwWorkThreadCount)
            dwIndex = 0;
    }
}
