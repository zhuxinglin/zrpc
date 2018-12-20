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

struct CRemoveServer
{
    const char *pszName;
    int iRet;
};

uint32_t g_dwWorkThreadCount = 0;
CGo *g_pGo = 0;
//
CNet::CNet() : m_oNetPool(sizeof(CNetEvent), 8)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

CNet::~CNet()
{
    CThread *pSch = CSchedule::GetObj();
    if (pSch)
        pSch->Release();

    CTaskQueue::Release();

    CCoroutine::Release();

    if (g_pGo)
    {
        for (uint32_t i = 0; i < g_dwWorkThreadCount; ++i)
            g_pGo[i].Exit();

        delete[] g_pGo;
    }
    g_pGo = 0;
    m_oNetPool.GetUse(this, &CNet::FreeListenFd);
    ERR_free_strings();
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
}

int CNet::Init(uint32_t dwWorkThread, uint32_t dwSp)
{
    if (dwWorkThread == 0)
        g_dwWorkThreadCount = sysconf(_SC_NPROCESSORS_CONF);
    else
        g_dwWorkThreadCount = dwWorkThread;

    if (g_dwWorkThreadCount == 1)
        g_dwWorkThreadCount = 2;
/*
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
*/
    if (Go() < 0)
        return -1;

    if (m_oEvent.Create() < 0)
    {
        m_sErr = m_oEvent.GetErr();
        return -1;
    }

    CSchedule *pSch = CSchedule::GetObj();
    if (pSch->Start() < 0)
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
        if (g_pGo[i].Start(0, i) < 0)
        {
            m_sErr = g_pGo[i].GetErr();
            return -1;
        }
    }
    return 0;
}

int CNet::Register(NEWOBJ(ITaskBase, pNewObj), void *pData, uint16_t wProtocol, uint16_t wPort, const char *pszIP,
                   uint16_t wVer, uint32_t dwTimeoutMs, const char *pszServerName, const char *pszSslCert, const char *pszSslKey)
{
    if (!pNewObj)
    {
        m_sErr = "ITaskBase *(*pNewObj)() is null";
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
        return SetUdpTask(pNewObj, pFd, pData, wProtocol, pszIP);

    CNetEvent *pEvent = (CNetEvent *)m_oNetPool.Malloc();
    if (!pEvent)
    {
        m_sErr = "new event node object fail";
        delete pFd;
        return -1;
    }
    pEvent->pNewObj = pNewObj;
    pEvent->pData = pData;
    pEvent->pFd = pFd;
    pEvent->wProtocol = wProtocol;
    pEvent->wVer = wVer;
    memcpy(pEvent->szServerName, pszServerName, sizeof(pEvent->szServerName));
    pEvent->dwSync = 0;
    pEvent->dwTimeoutUs = dwTimeoutMs * 1e3;

    iRet = m_oEvent.SetCtl(pFd->GetFd(), 0, 3, pEvent);
    if (iRet < 0)
    {
        m_sErr = m_oEvent.GetErr();
        delete pFd;
        m_oNetPool.Free(pEvent);
    }

    return iRet;
}

int CNet::Register(NEWOBJ(ITaskBase, pNewObj), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    if (wProtocol < ITaskBase::PROTOCOL_TIMER_FD || wProtocol > ITaskBase::PROTOCOL_EVENT_FD)
    {
        m_sErr = "error protocol";
        return -1;
    }

    ITaskBase *pTask = NewTask(pNewObj, pData, wProtocol, dwTimeoutUs);
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
            pTask->Release();
            return -1;
        }
        pNet->m_qwCid = ITaskBase::GenCid(pTimer->GetFd());
        strcpy(pNet->m_szAddr, "timer fd");
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
            pTask->Release();
            return -1;
        }
        pTask->m_qwCid = ITaskBase::GenCid(pEvent->GetFd());
        strcpy(pNet->m_szAddr, "event fd");
    }
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
        }
        else if (!pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue))
            break;

        iRet = 0;
    }while(0);
    if (iRet < 0)
    {
        pTaskQueue->DelTask((CTaskNode *)pTask->m_pTaskQueue);
        pTask->Release();
        m_sErr = "add message list fail";
    }
    return iRet;
}

int CNet::AddFdTask(ITaskBase *pTask, int iFd)
{
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    if (!pTaskQueue->AddWaitTask((CTaskNode *)pTask->m_pTaskQueue))
    {
        pTaskQueue->DelTask((CTaskNode *)pTask->m_pTaskQueue);
        pTask->Release();
        m_sErr = "add message list fail";
        return -1;
    }

    CThread *pSchedule = CSchedule::GetObj();
    if (pSchedule->PushMsg(iFd, 0, 0, (void *)pTask->m_qwCid) < 0)
    {
        m_sErr = pSchedule->GetErr();
        pTaskQueue->DelWaitTask((CTaskNode *)pTask->m_pTaskQueue);
        pTask->Release();
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
        int iRet = pTcp->Create(pszIP, wPort, 1024, wVer);
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

        int iRet = pTcps->Create(pszIP, wPort, 1024, pszSslCert, pszSslKey, wVer);
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

int CNet::SetUdpTask(NEWOBJ(ITaskBase, pNewObj), CFileFd* pFd, void *pData, uint16_t wProtocol, const char *pszIP)
{
    CNetTask *pTask = (CNetTask *)NewTask(pNewObj, pData, wProtocol);
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
        if (iLen > (int)sizeof(pTask->m_szAddr))
            iLen = sizeof(pTask->m_szAddr);
        strncpy(pTask->m_szAddr, pszIP, iLen);
    }

    if (AddFdTask(pTask, pFd->GetFd()) < 0)
        return -1;

    return 0;
}

ITaskBase *CNet::NewTask(NEWOBJ(ITaskBase, pNewObj), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    ITaskBase *pBase = pNewObj();
    if (!pBase)
    {
        m_sErr = "new task base object fail";
        return 0;
    }

    pBase->m_pNewObj = pNewObj;
    return NewTask(pBase, pData, wProtocol, dwTimeoutUs);
}

ITaskBase *CNet::NewTask(ITaskBase *pBase, void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs)
{
    pBase->m_pTaskQueue = CTaskQueue::GetObj()->Create(pBase);
    if (__builtin_expect(pBase->m_pTaskQueue == 0, 0))
    {
        m_sErr = "new task queue object fail";
        pBase->Release();
        return 0;
    }

    pBase->m_wProtocol = wProtocol;
    if (pBase->m_wRunStatus != ITaskBase::RUN_NOW)
        pBase->m_wRunStatus = ITaskBase::RUN_INIT;

    pBase->m_dwTimeout = dwTimeoutUs;
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
    struct epoll_event ev[256];
    CReliableFd oFd;
    CThread *pSchedule = CSchedule::GetObj();
    CTaskQueue *pTaskQueue = CTaskQueue::GetObj();
    CGo *pGo = g_pGo;
    while(true)
    {
        int iCount = m_oEvent.Wait(ev, 256, -1);
        if (iCount < 0)
        {
            break;
        }

        for (int i = 0; i < iCount; i++)
        {
            CNetEvent *pEvent = (CNetEvent*)ev[i].data.ptr;
            oFd.SetFd(pEvent->pFd->GetFd());

            CNetTask *pTask = (CNetTask *)NewTask(pEvent->pNewObj, pEvent->pData, pEvent->wProtocol, pEvent->dwTimeoutUs);
            if (!pTask)
            {
                CFileFd oClose(oFd.Accept());
                continue;
            }
            oFd.SetVer(pEvent->wVer);

            int iFd;
            if (pEvent->wProtocol != ITaskBase::PROTOCOL_TCP)
                iFd = oFd.Accept();
            else
                iFd = oFd.Accept(pTask->m_szAddr, sizeof(pTask->m_szAddr));

            if (iFd < 0)
            {
                pTask->Error("accpet fd fail");
                pTaskQueue->DelTask((CTaskNode*)pTask->m_pTaskQueue);
                pTask->Release();
                continue;
            }
            pTask->m_pFd = GetFileFd(pEvent->wProtocol, iFd);
            if (!pTask->m_pFd)
            {
                pTask->Error("create fd  objcet fail");
                pTaskQueue->DelTask((CTaskNode *)pTask->m_pTaskQueue);
                pTask->Release();
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
                pTaskQueue->DelTask((CTaskNode *)pTask->m_pTaskQueue);
                pTask->Release();
                continue;
            }

            uint32_t dwRunstatus = pTask->m_wRunStatus;
            iRet = pSchedule->PushMsg(iFd, 0, 0, (void *)pTask->m_qwCid);
            if (iRet < 0)
            {
                pTask->Error(pSchedule->GetErr().c_str());
                if (pTask->m_wRunStatus == ITaskBase::RUN_NOW)
                    pTask->m_wRunStatus = ITaskBase::RUN_EXIT;
                else
                {
                    pTaskQueue->DelWaitTask((CTaskNode*)pTask->m_pTaskQueue);
                    pTask->Release();
                }
                continue;
            }

            if (dwRunstatus == ITaskBase::RUN_NOW)
            {
                pTask->m_wRunStatus = ITaskBase::RUN_EXEC;
                uint32_t dwIndex = ITaskBase::GetSubCId(pTask->m_qwCid) % g_dwWorkThreadCount;
                pGo[dwIndex].PushMsg(0, 0, 0, 0);
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
        m_oEvent.SetCtl(pNet->pFd->GetFd(), 2, 0, 0);
        pNet->pFd->Close();

        CCoroutine *pCor = CCoroutine::GetObj();
        CNetTask *pNetTask = (CNetTask *)pCor->GetTaskBase();
        pNetTask->Yield(3000, -1, -1);
        delete pNet->pFd;
        pNet->pFd = 0;
        return -1;
    }

    return 0;
}
