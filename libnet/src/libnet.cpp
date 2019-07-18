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

using namespace znet;

struct CRemoveServer
{
    const char *pszName;
    int iRet;
};

CNet *CNet::m_pSelf = 0;
uint32_t g_dwWorkThreadCount = 2;
CGo *g_pGo = 0;
uint32_t g_dwMaxTaskCount = 100000;

//
CNet::CNet() : m_oNetPool(sizeof(CNetEvent), 8)
{
    m_bIsMainExit = true;
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

CNet::~CNet()
{
    if (!m_bIsMainExit)
        return;

    m_bIsMainExit = false;
    CThread *pSch = CSchedule::GetObj();
    if (pSch)
    {
        pSch->Exit();
        pSch->Release();
    }

    if (g_pGo)
    {
        for (uint32_t i = 0; i < g_dwWorkThreadCount; ++i)
            g_pGo[i].Exit([](void* p){
                for (uint32_t i = 0; i < g_dwWorkThreadCount; ++i)
                    CGoPost::Post();
                }
            );

        delete[] g_pGo;
    }
    g_pGo = 0;

    CTaskQueue::Release();
    CCoroutine::Release();

    m_oNetPool.GetUse(this, &CNet::FreeListenFd);

    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
    CFileFd& oFd(m_oEvent);
    oFd.Close();
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
        m_pSelf = pN;
}

void CNet::Release()
{
    if (m_pSelf)
        delete m_pSelf;
    m_pSelf = 0;
}


int CNet::Init(uint32_t dwWorkThread, uint32_t dwSp)
{
    if (dwWorkThread == 0)
        g_dwWorkThreadCount = sysconf(_SC_NPROCESSORS_CONF);
    else
        g_dwWorkThreadCount = dwWorkThread;

    if (g_dwWorkThreadCount == 1)
        g_dwWorkThreadCount = 2;

    if (!CCoroutine::GetObj())
    {
        m_sErr = "get coroutine object fail";
        return -1;
    }

    if (dwSp != 0)
        CCoroutine::GetObj()->SetRspSize(dwSp);

    if (!CTaskQueue::GetObj())
    {
        m_sErr = "get task queue object fail";
        return -1;
    }

    if (Go() < 0)
        return -1;

    if (m_oEvent.Create() < 0)
    {
        m_sErr = m_oEvent.GetErr();
        return -1;
    }

    CSchedule *pSch = CSchedule::GetObj();
    if (pSch->Start("schedule_thread") < 0)
    {
        m_sErr = pSch->GetErr();
        return -1;
    }
    return 0;
}

int CNet::Go()
{
    g_pGo = new (std::nothrow) CGo[g_dwWorkThreadCount];
    if (!g_pGo)
    {
        m_sErr = "create go thread object failed";
        return -1;
    }

    for (uint32_t i = 0; i < g_dwWorkThreadCount; ++i)
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "go_thread_%d", i);
        if (g_pGo[i].Start(buf, false, 0, i) < 0)
        {
            m_sErr = g_pGo[i].GetErr();
            return -1;
        }
    }
    return 0;
}

void CNet::SetMaxTaskCount(uint32_t dwMaxTaskCount)
{
    g_dwMaxTaskCount = dwMaxTaskCount;
}

uint32_t CNet::GetCurTaskCount() const
{
    return CTaskQueue::GetObj()->GetCurTaskCount();
}

uint32_t CNet::GetTaskThreadCount() const
{
    return g_dwWorkThreadCount;
}

int CNet::Register(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint16_t wPort, const char *pszIP,
                   uint16_t wVer, uint32_t dwTimeoutMs, const char *pszServerName, const char *pszSslCert, const char *pszSslKey)
{
    if (!pCb)
    {
        m_sErr = "ITaskBase *(*pCb)() is null";
        return -1;
    }

    if (wProtocol > ITaskBase::PROTOCOL_UDPG)
    {
        m_sErr = "not net register";
        return -1;
    }

    int iRet;
    CFileFd *pFd = GetFd(wProtocol, wPort, pszIP, wVer, pszSslCert, pszSslKey);
    if (!pFd)
        return -1;

    if (wProtocol > ITaskBase::PROTOCOL_TCPS)
        return SetUdpTask(pCb, pFd, pData, wProtocol, pszIP);

    CNetEvent *pEvent = (CNetEvent *)m_oNetPool.Malloc();
    if (!pEvent)
    {
        m_sErr = "new event node object fail";
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

    iRet = m_oEvent.SetCtl(pFd->GetFd(), 0, CEventEpoll::EPOLL_IN, pEvent);
    if (iRet < 0)
    {
        m_sErr = m_oEvent.GetErr();
        delete pFd;
        m_oNetPool.Free(pEvent);
    }

    return iRet;
}

int CNet::Register(NEWOBJ(ITaskBase, pCb), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    if (wProtocol < ITaskBase::PROTOCOL_TIMER_FD || wProtocol > ITaskBase::PROTOCOL_EVENT_FD)
    {
        m_sErr = "error protocol";
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
            m_sErr = "new timer fd object fail";
            return -1;
        }
        pNet = (CNetTask *)pTask;
        pNet->m_pFd = pTimer;
        if (pTimer->Create(dwTimeoutUs) < 0)
        {
            m_sErr = pTimer->GetErr();
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
            m_sErr = "new event fd object fail";
            return -1;
        }
        pNet = (CNetTask *)pTask;
        pNet->m_pFd = pEvent;
        if (pEvent->Create() < 0)
        {
            m_sErr = pEvent->GetErr();
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
        m_sErr = "error protocol";
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
        m_sErr = "error protocol";
        return -1;
    }

    ITaskBase* pTask = pCb();
    if (!pTask)
    {
        m_sErr = "new task base object fail";
        return -1;
    }

    return Register(pTask, pData, wProtocol, iFd, dwTimeoutMs);
}

int CNet::AddTimerTask(ITaskBase *pTask, uint32_t dwTimeout)
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
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
        m_sErr = "add message list fail";
    }
    return iRet;
}

int CNet::AddFdTask(ITaskBase *pTask, int iFd)
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    if (!pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue))
    {
        DeleteTask(pTask);
        m_sErr = "add message list fail";
        return -1;
    }

    CThread *pSchedule = CSchedule::GetObj();
    if (pSchedule->PushMsg(iFd, 0, 0, (void *)pTask->m_qwCid) < 0)
    {
        m_sErr = pSchedule->GetErr();
        DeleteObj(pTask);
        return -1;
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
            m_sErr = "Create tcp object failed";
            break;
        }
        int iRet = pTcp->Create(pszIP, wPort, 10240, wVer);
        if (iRet < 0)
        {
            m_sErr = pTcp->GetErr();
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
            m_sErr = "Create tcp ssl object failed";
            break;
        }

        int iRet = pTcps->Create(pszIP, wPort, 10240, pszSslCert, pszSslKey, wVer);
        if (iRet < 0)
        {
            m_sErr = pTcps->GetErr();
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
            m_sErr = "Create unix object failed";
            break;
        }
        int iRet = pUnix->Create(pszIP, 1024);
        if (iRet < 0)
        {
            m_sErr = pUnix->GetErr();
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
            m_sErr = "Create unix object failed";
            break;
        }
        int iRet = pUdp->Create(pszIP, wPort, 0, wVer);
        if (iRet < 0)
        {
            m_sErr = pUdp->GetErr();
            delete pUdp;
            break;
        }
        pFd = pUdp;
    }
    break;

    case ITaskBase::PROTOCOL_UDPG:
        m_sErr = "not udp group";
        break;

    default:
        m_sErr = "error protocol";
        break;
    }
    return pFd;
}

int CNet::SetUdpTask(NEWOBJ(ITaskBase, pCb), CFileFd* pFd, void *pData, uint16_t wProtocol, const char *pszIP)
{
    CNetTask *pTask = (CNetTask *)NewTask(pCb, pData, wProtocol);
    if (!pTask)
    {
        m_sErr = "new task base object fail";
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
        m_sErr = "new task base object fail";
        return 0;
    }

    pBase->m_pCb = pCb;
    return NewTask(pBase, pData, wProtocol, dwTimeoutUs);
}

ITaskBase *CNet::NewTask(ITaskBase *pBase, void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    pBase->m_pTaskQueue = CTaskQueue::GetObj()->Create(pBase);
    if (__builtin_expect(pBase->m_pTaskQueue == 0, 0))
    {
        m_sErr = "new task queue object fail";
        DeleteObj(pBase);
        return 0;
    }

    pBase->m_wProtocol = wProtocol;
    if (pBase->m_wRunStatus != ITaskBase::RUN_NOW)
        pBase->m_wRunStatus = ITaskBase::RUN_INIT;

    pBase->m_dwTimeout = dwTimeoutUs;
    pBase->m_qwConnectTime = CTimerFd::GetUs();
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
    CThread *pSchedule = CSchedule::GetObj();
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    while(m_bIsMainExit)
    {
        int iCount = m_oEvent.Wait(ev, 512, -1);
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
            if (pEvent->wProtocol != ITaskBase::PROTOCOL_TCP && pEvent->wProtocol != ITaskBase::PROTOCOL_TCPS)
                iFd = oFd.Accept();
            else
                iFd = oFd.Accept(pTask->m_szAddr, sizeof(pTask->m_szAddr) - 1);

            if (pTaskQueue->GetCurTaskCount() > g_dwMaxTaskCount)
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
                    if (!pTaskQueue->AddExecTask((CTaskNode *)pTask->m_pTaskQueue))
                        break;
                }
                else if (!pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue)) // 添加到等待队列
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
            iRet = pSchedule->PushMsg(iFd, CEventEpoll::EPOLL_ADD, CEventEpoll::EPOLL_ET_IN, (void *)pTask->m_qwCid);
            if (iRet < 0)
            {
                pTask->Error(pSchedule->GetErr().c_str());
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

    m_oNetPool.GetUse(this, &CNet::RemoveServer, &oRemove);
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
        m_oEvent.SetCtl(pNet->pFd->GetFd(), CEventEpoll::EPOLL_DEL, 0, 0);
        pNet->pFd->Close();

        CCoroutine *pCor = CCoroutine::GetObj();
        CNetTask *pNetTask = (CNetTask *)pCor->GetTaskBase();
        pNetTask->Yield(3e3);
        delete pNet->pFd;
        pNet->pFd = 0;
        return -1;
    }

    return 0;
}

void CNet::DeleteTask(ITaskBase* pTask)
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    pTaskQueue->DelTask((CTaskNode *)pTask->m_pTaskQueue);
    pTask->m_pTaskQueue = 0;
    DeleteObj(pTask);
}

void CNet::DeleteObj(ITaskBase* pTask)
{
    std::shared_ptr<ITaskBase> optr(pTask->m_oPtr);
    pTask->m_oPtr = nullptr;
}

