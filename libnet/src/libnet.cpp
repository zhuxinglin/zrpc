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

#include "libnet.h"
#include "socket_fd.h"
#include "coroutine.h"
#include "net_struct.h"
#include "schedule.h"
#include "task_queue.h"
#include "net_task.h"
#include "go.h"
#include <sched.h>
#include <time.h>
#include "timer_fd.h"
#include "go_post.h"
#include "context.h"

using namespace znet;

struct CRemoveServer
{
    const char *pszName;
    int iRet;
};

CNet *CNet::m_pSelf = 0;
CContext* g_pContext = 0;

//
CNet::CNet()
{
    m_bIsMainExit = true;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
    m_pContext = nullptr;
}

CNet::~CNet()
{
    m_bIsMainExit = false;
    if (g_pContext)
        g_pContext->m_oNetPool.GetUse(this, &CNet::FreeListenFd);

    if (!g_pContext)
        return;

    delete g_pContext;
    g_pContext = nullptr;
}

CNet *CNet::GetObj()
{
    if (!m_pSelf)
        m_pSelf = new (std::nothrow) CNet;
    return m_pSelf;
}

void CNet::Set(CNet* pN)
{
    if (!m_pSelf)
    {
        m_pSelf = pN;
        g_pContext = reinterpret_cast<CContext*>(m_pSelf->m_pContext);
    }
}

void CNet::Release()
{
    if (m_pSelf)
        delete m_pSelf;
    m_pSelf = 0;
}

const char* CNet::GetErr()
{
    if (g_pContext)
        return g_pContext->m_sErr.c_str();
    return "create context object failed!";
}

int CNet::Init(uint32_t dwWorkThread, uint32_t dwSp)
{
    if (dwWorkThread == 0)
        dwWorkThread = sysconf(_SC_NPROCESSORS_CONF);

    if (dwWorkThread == 1)
        dwWorkThread = 2;

    g_pContext = new (std::nothrow) CContext;
    if (!g_pContext)
        return -1;

    m_pContext = g_pContext;
    CContext* pCx = g_pContext;
    if (pCx->Init(dwWorkThread) < 0)
        return -1;

    if (dwSp != 0)
        pCx->m_pCo->SetRspSize(dwSp);

    if (Go() < 0)
        return -1;

    if (pCx->m_oEvent.Create() < 0)
    {
        pCx->m_sErr = pCx->m_oEvent.GetErr();
        return -1;
    }

    if (pCx->m_pSchedule->Start("schedule_thread") < 0)
    {
        pCx->m_sErr = pCx->m_pSchedule->GetErr();
        return -1;
    }
    return 0;
}

int CNet::Go()
{
    for (uint32_t i = 0; i < g_pContext->m_dwWorkThreadCount; ++i)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "go_thread_%d", i);
        if (g_pContext->m_pGo[i].Start(buf, false, 0, i) < 0)
        {
            g_pContext->m_sErr = g_pContext->m_pGo[i].GetErr();
            return -1;
        }
    }
    return 0;
}

void CNet::SetMaxTaskCount(uint32_t dwMaxTaskCount)
{
    g_pContext->m_dwMaxTaskCount = dwMaxTaskCount;
}

uint32_t CNet::GetCurTaskCount() const
{
    return g_pContext->m_pTaskQueue->GetCurTaskCount();
}

uint32_t CNet::GetTaskThreadCount() const
{
    return g_pContext->m_dwWorkThreadCount;
}

uint64_t CNet::GetCurCid() const
{
    CContext* pCx = g_pContext;
    if (!pCx || !pCx->m_pCo || !pCx->m_pCo->GetTaskBase())
        return 0;
    return pCx->m_pCo->GetTaskBase()->m_qwCid;
}

ITaskBase *CNet::GetCurTask()
{
    if (!g_pContext || g_pContext->m_pCo)
        return nullptr;
    return g_pContext->m_pCo->GetTaskBase();
}

int CNet::Register(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint16_t wPort, const char *pszIP,
                   uint16_t wVer, uint32_t dwTimeoutMs, const char *pszServerName, const char *pszSslCert, const char *pszSslKey)
{
    CContext* pCx = g_pContext;
    if (!pCb)
    {
        pCx->m_sErr = "ITaskBase *(*pCb)() is null";
        return -1;
    }

    if (wProtocol > ITaskBase::PROTOCOL_UDPG)
    {
        pCx->m_sErr = "not net register";
        return -1;
    }

    int iRet;
    CFileFd *pFd = GetFd(wProtocol, wPort, pszIP, wVer, pszSslCert, pszSslKey);
    if (!pFd)
        return -1;

    if (wProtocol > ITaskBase::PROTOCOL_TCPS)
        return SetUdpTask(pCb, pFd, pData, wProtocol, pszIP);

    CNetEvent *pEvent = (CNetEvent *)pCx->m_oNetPool.Malloc();
    if (!pEvent)
    {
        pCx->m_sErr = "new event node object fail";
        delete pFd;
        return -1;
    }
    pEvent->pCb = pCb;
    pEvent->pData = pData;
    pEvent->pFd = pFd;
    pEvent->wProtocol = wProtocol;
    pEvent->wVer = wVer;
    pEvent->dwSync = 0;
    pEvent->dwTimeoutUs = dwTimeoutMs * 1e3;

    int iSvcNameLen = strlen(pszServerName);
    if (iSvcNameLen > (int)(sizeof(pEvent->szServerName) - 1))
        iSvcNameLen = sizeof(pEvent->szServerName) - 1;
    memset(pEvent->szServerName, 0, sizeof(pEvent->szServerName));
    memcpy(pEvent->szServerName, pszServerName, iSvcNameLen);

    iRet = pCx->m_oEvent.SetCtl(pFd->GetFd(), 0, CEventEpoll::EPOLL_IN, pEvent);
    if (iRet < 0)
    {
        pCx->m_sErr = pCx->m_oEvent.GetErr();
        delete pFd;
        pCx->m_oNetPool.Free(pEvent);
    }

    return iRet;
}

int CNet::Register(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    CContext* pCx = g_pContext;
    if (wProtocol < ITaskBase::PROTOCOL_TIMER_FD || wProtocol > ITaskBase::PROTOCOL_EVENT_FD)
    {
        pCx->m_sErr = "error protocol";
        return -1;
    }

    ITaskBase *pTask = NewTask(pCb, pData, wProtocol, dwTimeoutUs);
    if (!pTask)
        return -1;

    CNetTask *pNet;
    switch (wProtocol)
    {
    case ITaskBase::PROTOCOL_TIMER_FD:
    {
        CTimerFd *pTimer = new (std::nothrow) CTimerFd();
        if (!pTimer)
        {
            pCx->m_sErr = "new timer fd object fail";
            return -1;
        }
        pNet = (CNetTask *)pTask;
        pNet->m_pFd = pTimer;
        if (pTimer->Create(dwTimeoutUs) < 0)
        {
            pCx->m_sErr = pTimer->GetErr();
            DeleteTask(pTask);
            return -1;
        }
        pNet->m_qwCid = ITaskBase::GenCid(pTimer->GetFd());
        strcpy(pNet->m_szAddr, "timer_fd");
    }
    break;

    case ITaskBase::PROTOCOL_EVENT_FD:
    {
        CEventFd *pEvent = new (std::nothrow) CEventFd();
        if (!pEvent)
        {
            pCx->m_sErr = "new event fd object fail";
            return -1;
        }
        pNet = (CNetTask *)pTask;
        pNet->m_pFd = pEvent;
        if (pEvent->Create() < 0)
        {
            pCx->m_sErr = pEvent->GetErr();
            DeleteTask(pTask);
            return -1;
        }
        pTask->m_qwCid = ITaskBase::GenCid(pEvent->GetFd());
        strcpy(pNet->m_szAddr, "event_fd");
    }
    break;
    default:
        DeleteTask(pTask);
        return -1;
    break;
    }

    return AddFdTask(pTask, pNet->m_pFd->GetFd());
}

int CNet::Register(ITaskBase *pBase, void *pData, uint16_t wProtocol, int iFd, uint32_t dwTimeoutMs)
{
    if (wProtocol != ITaskBase::PROTOCOL_TIMER)
    {
        g_pContext->m_sErr = "error protocol";
        delete pBase;
        return -1;
    }

    if (dwTimeoutMs != (uint32_t)-1)
        dwTimeoutMs = dwTimeoutMs * 1e3;

    if (!NewTask(pBase, pData, wProtocol, dwTimeoutMs))
        return -1;
    pBase->m_qwCid = ITaskBase::GenCid(iFd);

    if (iFd == -1)
        return AddTimerTask(pBase, dwTimeoutMs);
    return AddFdTask(pBase, iFd);
}

int CNet::Register(NEWOBJ(ITaskBase, pCb), void* pData, uint16_t wProtocol, int iFd, uint32_t dwTimeoutMs)
{
    if (wProtocol != ITaskBase::PROTOCOL_TIMER)
    {
        g_pContext->m_sErr = "error protocol";
        return -1;
    }

    ITaskBase* pTask = pCb();
    if (!pTask)
    {
        g_pContext->m_sErr = "new task base object fail";
        return -1;
    }

    return Register(pTask, pData, wProtocol, iFd, dwTimeoutMs);
}

int CNet::AddTimerTask(ITaskBase *pTask, uint32_t dwTimeout)
{
    CTaskQueue *pTaskQueue = g_pContext->m_pTaskQueue;
    int iRet = -1;
    do
    {
        if (dwTimeout == 0)
        {
            if (!pTaskQueue->AddExecTask((CTaskNode *)pTask->m_pTaskQueue))
                break;
            CGoPost::Post();
        }
        else if (!pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue))
            break;

        iRet = 0;
    }while(0);
    if (iRet < 0)
    {
        DeleteTask(pTask);
        g_pContext->m_sErr = "add message list fail";
    }
    return iRet;
}

int CNet::AddFdTask(ITaskBase *pTask, int iFd)
{
    CTaskQueue *pTaskQueue = g_pContext->m_pTaskQueue;
    if (pTask->m_wRunStatus == ITaskBase::RUN_NOW)
    {
        if (!pTaskQueue->AddExecTask((CTaskNode *)pTask->m_pTaskQueue))
            return -1;
    }
    else
    {
        if (!pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue))
        {
            DeleteTask(pTask);
            g_pContext->m_sErr = "add message list fail";
            return -1;
        }
    }

    CThread *pSchedule = g_pContext->m_pSchedule;
    uint8_t wRunstatus = pTask->m_wRunStatus;
    if (pSchedule->PushMsg(iFd, 0, 0, (void *)pTask->m_qwCid) < 0)
    {
        g_pContext->m_sErr = pSchedule->GetErr();
        if (pTask->m_wRunStatus == ITaskBase::RUN_NOW)
        {
            pTask->m_wRunStatus = ITaskBase::RUN_EXIT;
            CGoPost::Post();
        }
        else
            DeleteObj(pTask);
        return -1;
    }

    if (wRunstatus == ITaskBase::RUN_NOW)
    {
        pTask->m_wRunStatus = ITaskBase::RUN_EXEC;
        CGoPost::Post();
    }

    return 0;
}

CFileFd *CNet::GetFd(uint16_t wProtocol, uint16_t wPort, const char *pszIP, uint16_t wVer, const char *pszSslCert, const char *pszSslKey)
{
    CFileFd* pFd = 0;
    switch (wProtocol)
    {
    case ITaskBase::PROTOCOL_TCP:
    {
        CTcpSvc *pTcp = new (std::nothrow) CTcpSvc();
        if (!pTcp)
        {
            g_pContext->m_sErr = "Create tcp object failed";
            break;
        }
        int iRet = pTcp->Create(pszIP, wPort, 10240, wVer);
        if (iRet < 0)
        {
            g_pContext->m_sErr = pTcp->GetErr();
            delete pTcp;
            break;
        }
        pFd = pTcp;
    }
    break;

    case ITaskBase::PROTOCOL_TCPS:
    {
        CTcpsSvc* pTcps = new (std::nothrow) CTcpsSvc();
        if (!pTcps)
        {
            g_pContext->m_sErr = "Create tcp ssl object failed";
            break;
        }

        int iRet = pTcps->Create(pszIP, wPort, 10240, pszSslCert, pszSslKey, wVer);
        if (iRet < 0)
        {
            g_pContext->m_sErr = pTcps->GetErr();
            delete pTcps;
            break;
        }
        pFd = pTcps;
    }
    break;

    case ITaskBase::PROTOCOL_UNIX:
    {
        CUnixSvc *pUnix;
        pUnix = new (std::nothrow) CUnixSvc();
        if (!pUnix)
        {
            g_pContext->m_sErr = "Create unix object failed";
            break;
        }
        int iRet = pUnix->Create(pszIP, 1024);
        if (iRet < 0)
        {
            g_pContext->m_sErr = pUnix->GetErr();
            delete pUnix;
            break;
        }
        pFd = pUnix;
    }
    break;

    case ITaskBase::PROTOCOL_UDP:
    {
        CUdpSvc* pUdp = new (std::nothrow) CUdpSvc();
        if (!pUdp)
        {
            g_pContext->m_sErr = "Create unix object failed";
            break;
        }
        int iRet = pUdp->Create(pszIP, wPort, 0, wVer);
        if (iRet < 0)
        {
            g_pContext->m_sErr = pUdp->GetErr();
            delete pUdp;
            break;
        }
        pFd = pUdp;
    }
    break;

    case ITaskBase::PROTOCOL_UDPG:
        g_pContext->m_sErr = "not udp group";
        break;

    default:
        g_pContext->m_sErr = "error protocol";
        break;
    }
    return pFd;
}

int CNet::SetUdpTask(NEWOBJ(ITaskBase, pCb), CFileFd* pFd, void *pData, uint16_t wProtocol, const char *pszIP)
{
    CNetTask *pTask = (CNetTask *)NewTask(pCb, pData, wProtocol);
    if (!pTask)
    {
        g_pContext->m_sErr = "new task base object fail";
        delete pFd;
        return -1;
    }
    pTask->m_pFd = pFd;
    pTask->m_qwCid = ITaskBase::GenCid(pFd->GetFd());
    if (pszIP)
    {
        int iLen = strlen(pszIP);
        if (iLen > (int)(sizeof(pTask->m_szAddr) - 1))
            iLen = sizeof(pTask->m_szAddr) - 1;
        strncpy(pTask->m_szAddr, pszIP, iLen);
    }

    if (AddFdTask(pTask, pFd->GetFd()) < 0)
        return -1;

    return 0;
}

ITaskBase *CNet::NewTask(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    ITaskBase *pBase = pCb();
    if (!pBase)
    {
        g_pContext->m_sErr = "new task base object fail";
        return 0;
    }

    pBase->m_pCb = pCb;
    return NewTask(pBase, pData, wProtocol, dwTimeoutUs);
}

ITaskBase *CNet::NewTask(ITaskBase *pBase, void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    pBase->m_pTaskQueue = g_pContext->m_pTaskQueue->Create(pBase);
    if (__builtin_expect(pBase->m_pTaskQueue == 0, 0))
    {
        g_pContext->m_sErr = "new task queue object fail";
        DeleteObj(pBase);
        return 0;
    }

    pBase->m_wProtocol = wProtocol;
    if (pBase->m_wRunStatus != ITaskBase::RUN_NOW)
        pBase->m_wRunStatus = ITaskBase::RUN_INIT;

    pBase->m_dwTimeout = dwTimeoutUs;
    pBase->m_qwConnectTime = CTimerFd::GetNs();
    pBase->m_pData = pData;
    return pBase;
}

CFileFd *CNet::GetFileFd(uint16_t wProtocol, int iFd)
{
    CFileFd* pFd = 0;
    switch(wProtocol)
    {
    case ITaskBase::PROTOCOL_TCP:
        pFd = new (std::nothrow) CTcpSvc();
    break;

    case ITaskBase::PROTOCOL_TCPS:
        pFd = new (std::nothrow) CTcpsReliableFd();
    break;

    case ITaskBase::PROTOCOL_UNIX:
        pFd = new (std::nothrow) CUnixSvc();
    break;

    case ITaskBase::PROTOCOL_UDPG:
    case ITaskBase::PROTOCOL_UDP:
        pFd = new (std::nothrow) CUdpSvc();
    break;
    }
    pFd->SetFd(iFd);
    return pFd;
}

int CNet::Start()
{
    struct epoll_event ev[512];
    CReliableFd oFd;
    CContext* pCx = g_pContext;
    while(m_bIsMainExit)
    {
        int iCount = pCx->m_oEvent.Wait(ev, 512, -1);
        if (iCount < 0)
        {
            break;
        }

        for (int i = 0; i < iCount; i++)
        {
            CNetEvent *pEvent = (CNetEvent*)ev[i].data.ptr;
            oFd.SetFd(pEvent->pFd->GetFd());

            CNetTask *pTask = (CNetTask *)NewTask(pEvent->pCb, pEvent->pData, pEvent->wProtocol, pEvent->dwTimeoutUs);
            if (!pTask)
            {
                CFileFd oClose(oFd.Accept());
                continue;
            }
            oFd.SetVer(pEvent->wVer);
            pTask->m_sServerName = pEvent->szServerName;

            int iFd;
            if (pEvent->wProtocol == ITaskBase::PROTOCOL_UNIX)
                iFd = oFd.Accept();
            else
                iFd = oFd.Accept(pTask->m_szAddr, sizeof(pTask->m_szAddr) - 1);

            if (pCx->m_pTaskQueue->GetCurTaskCount() > pCx->m_dwMaxTaskCount)
            {
                CFileFd oClose(iFd);
                iFd = -1;
            }

            if (iFd < 0)
            {
                pTask->Error("accpet fd fail");
                DeleteTask(pTask);
                continue;
            }
            pTask->m_pFd = GetFileFd(pEvent->wProtocol, iFd);
            if (!pTask->m_pFd)
            {
                pTask->Error("create fd  objcet fail");
                DeleteTask(pTask);
                CFileFd oClose(iFd);
                continue;
            }

            if (pEvent->wProtocol == ITaskBase::PROTOCOL_TCPS)
            {
                CTcpsReliableFd *pTcps = (CTcpsReliableFd *)pTask->m_pFd;
                pTcps->SetCtx((CTcpsReliableFd*)pEvent->pFd);
            }

            pTask->m_qwCid = ITaskBase::GenCid(iFd);

            int iRet = -1;
            do
            {
                // 添加到执行队列
                if (pTask->m_wRunStatus == ITaskBase::RUN_NOW)
                {
                    if (!pCx->m_pTaskQueue->AddExecTask((CTaskNode *)pTask->m_pTaskQueue))
                        break;
                }
                else if (!pCx->m_pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue)) // 添加到等待队列
                    break;
                iRet = 0;
            }while(0);

            if (iRet < 0)
            {
                pTask->Error("add message list fail");
                DeleteTask(pTask);
                continue;
            }

            uint32_t dwRunstatus = pTask->m_wRunStatus;
            iRet = pCx->m_pSchedule->PushMsg(iFd, CEventEpoll::EPOLL_ADD, CEventEpoll::EPOLL_ET_IN, (void *)pTask->m_qwCid);
            if (iRet < 0)
            {
                pTask->Error(pCx->m_pSchedule->GetErr().c_str());
                if (pTask->m_wRunStatus == ITaskBase::RUN_NOW)
                {
                    pTask->m_wRunStatus = ITaskBase::RUN_EXIT;
                    CGoPost::Post();
                }
                else
                    DeleteObj(pTask);
                continue;
            }

            if (dwRunstatus == ITaskBase::RUN_NOW)
            {
                pTask->m_wRunStatus = ITaskBase::RUN_EXEC;
                CGoPost::Post();
            }
        }
    }
    return 0;
}

void CNet::Stop()
{
    m_bIsMainExit = false;
    if (!g_pContext)
        return;

    CFileFd &oFd(g_pContext->m_oEvent);
    oFd.Close();
}

int CNet::FreeListenFd(void *pEvent, void* pData)
{
    CNetEvent *pNet = (CNetEvent*)pEvent;
    delete pNet->pFd;
    return 0;
}

int CNet::Unregister(const char *pszServerName)
{
    CRemoveServer oRemove;
    oRemove.pszName = pszServerName;
    oRemove.iRet = -1;

    g_pContext->m_oNetPool.GetUse(this, &CNet::RemoveServer, &oRemove);
    return oRemove.iRet;
}

int CNet::RemoveServer(void *pEvent, void *pData)
{
    CRemoveServer *pRemove = (CRemoveServer *)pData;
    CNetEvent *pNet = (CNetEvent *)pEvent;

    while (__sync_lock_test_and_set(&pNet->dwSync, 1))
    {
        pRemove->iRet = 0;
        return 0;
    }

    if (strcmp(pNet->szServerName, pRemove->pszName) == 0)
    {
        pRemove->iRet = 0;
        g_pContext->m_oEvent.SetCtl(pNet->pFd->GetFd(), CEventEpoll::EPOLL_DEL, 0, 0);
        pNet->pFd->Close();

        CNetTask *pNetTask = (CNetTask *)g_pContext->m_pCo->GetTaskBase();
        pNetTask->Yield(3e3);
        delete pNet->pFd;
        pNet->pFd = 0;
        return -1;
    }

    return 0;
}

void CNet::DeleteTask(ITaskBase* pTask)
{
    g_pContext->m_pTaskQueue->DelTask((CTaskNode *)pTask->m_pTaskQueue);
    pTask->m_pTaskQueue = 0;
    DeleteObj(pTask);
}

void CNet::DeleteObj(ITaskBase* pTask)
{
    std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
    pTask->m_oPtr = nullptr;
}

void CNet::ExitCo(uint64_t qwCid)
{
    if (qwCid == 0)
        qwCid = GetCurCid();

    if (!g_pContext || !g_pContext->m_pTaskQueue)
        return;

    g_pContext->m_pTaskQueue->ExitTask(qwCid);
    CGoPost::Post();
}

bool CNet::IsExitCo(uint64_t qwCid) const
{
    CContext* pCx = g_pContext;
    if (!pCx)
        return true;
    if (qwCid == 0)
    {
        if (!pCx->m_pCo || !pCx->m_pCo->GetTaskBase())
            return true;
        return pCx->m_pCo->GetTaskBase()->IsExitCo();
    }

    if (!pCx->m_pTaskQueue)
        return true;

    return pCx->m_pTaskQueue->IsExitTask(qwCid);
}
