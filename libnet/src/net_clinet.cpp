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

#include "net_client.h"
#include "coroutine.h"
#include "net_task.h"
#include "task_queue.h"
#include "schedule.h"
#include "thread.h"

using namespace znet;

CNetClient::CNetClient() : m_dwConnTimeout(3000),
                           m_wAddrLen(0),
                           m_pFd(0)
{
    m_dwCloseRef = 0;
}

CNetClient::~CNetClient()
{
    if (m_pFd)
        delete m_pFd;
    m_pFd = 0;
}

int CNetClient::Connect(const char *pszAddr, uint16_t wPort, uint16_t wProtocol, uint16_t wVer)
{
    if (m_pFd)
        return -1;

    m_dwCloseRef = 1;
    m_wProtocol = wProtocol;
    m_wVer = wVer;
    int iRet = -1;
    if (wProtocol == ITaskBase::PROTOCOL_TCP)
        iRet = TcpConnect(pszAddr, wPort);
    else if (wProtocol == ITaskBase::PROTOCOL_UNIX)
        iRet = UnixConnect(pszAddr);
    else if (wProtocol <= ITaskBase::PROTOCOL_UDPG)
        iRet = UdpConnect(pszAddr, wPort);

    if (iRet < 0)
        Close();

    return iRet;
}

void CNetClient::Close()
{
    IsClose();
}

int CNetClient::Read(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    Reference oRef(this);
    if (!m_pFd || m_pFd->GetFd() < 0)
        return -1;

    int iRet = -1;
    if (m_wProtocol <= ITaskBase::PROTOCOL_UNIX)
        iRet = ReadReliable(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol == ITaskBase::PROTOCOL_TCPS)
        iRet = TcpsRead(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol <= ITaskBase::PROTOCOL_UDPG)
        iRet = ReadUnreliable(pszBuf, iLen, dwTimeoutMs);
    return iRet;
}

int CNetClient::Write(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    Reference oRef(this);
    if (!m_pFd || m_pFd->GetFd() < 0)
        return -1;

    int iRet = -1;
    if (m_wProtocol <= ITaskBase::PROTOCOL_UNIX)
        iRet = WriteReliable(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol == ITaskBase::PROTOCOL_TCPS)
        iRet = TcpsWrite(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol <= ITaskBase::PROTOCOL_UDPG)
        iRet = WriteUnreliable(pszBuf, iLen, dwTimeoutMs);
    return iRet;
}

int CNetClient::TcpConnect(const char *pszAddr, uint16_t wPort)
{
    CTcpCli *pCli = new (std::nothrow) CTcpCli();
    if (!pCli)
    {
        m_pFd->SetErr("new tcp object failed");
        return -1;
    }
    m_pFd = pCli;
    CCoroutine *pCor = CCoroutine::GetObj();
    return pCli->Create(pszAddr, wPort, m_dwConnTimeout, pCor->GetTaskBase(), m_wVer);
}

int CNetClient::UdpConnect(const char *pszAddr, uint16_t wPort)
{
    CUdpCli *pCli = new (std::nothrow) CUdpCli();
    if (!pCli)
    {
        m_pFd->SetErr("new udp object failed");
        return -1;
    }
    m_pFd = pCli;
    return pCli->Create(pszAddr, wPort, m_szUdpAddr, (uint32_t *)&m_wAddrLen, m_wVer);
}

int CNetClient::UnixConnect(const char *pszAddr)
{
    CUnixCli *pCli = new (std::nothrow) CUnixCli();
    if (!pCli)
    {
        m_pFd->SetErr("new unix object failed");
        return -1;
    }
    m_pFd = pCli;
    CCoroutine *pCor = CCoroutine::GetObj();
    return pCli->Create(pszAddr, m_dwConnTimeout, pCor->GetTaskBase());
}

int CNetClient::ReadReliable(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CReliableFd *pSock = (CReliableFd *)m_pFd;
    int iRet = pSock->Read(pszBuf, iLen);
    if (iRet == 0)
        iRet = Wait(ITaskBase::YIELD_ET_IN, dwTimeoutMs);
    return iRet;
}

int CNetClient::ReadUnreliable(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CUnreliableFd* pSock = (CUnreliableFd *)m_pFd;
    m_wAddrLen = sizeof(m_szUdpAddr);
    int iRet = pSock->Read(pszBuf, iLen, (struct sockaddr *)m_szUdpAddr, (uint32_t *)&m_wAddrLen);
    if (iRet == 0)
        iRet = Wait(ITaskBase::YIELD_ET_IN, dwTimeoutMs);

    return iRet;
}

int CNetClient::WriteReliable(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CReliableFd* pSock = (CReliableFd *)m_pFd;
    int iOffset = 0;
    CPadlock<CCoLock> ol(m_oLock);
    while (iLen > 0)
    {
        int iRet = pSock->Write(pszBuf + iOffset, iLen);
        if (iRet < 0)
            return -1;

        if (iRet == 0)
        {
            if ((iRet = Wait(ITaskBase::YIELD_ET_OUT, dwTimeoutMs)) < 0)
                return iRet;
            continue;
        }

        iOffset += iRet;
        iLen -= iRet;
    }

    return iOffset;
}

int CNetClient::WriteUnreliable(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CUnreliableFd* pSock = (CUnreliableFd *)m_pFd;
    int iOffset = 0;
    CPadlock<CCoLock> ol(m_oLock);
    while (iLen > 0)
    {
        int iRet = pSock->Write(pszBuf + iOffset, iLen, (struct sockaddr *)m_szUdpAddr, m_wAddrLen);
        if (iRet < 0)
            return -1;

        if (iRet == 0)
        {
            if ((iRet = Wait(ITaskBase::YIELD_ET_OUT, dwTimeoutMs)) < 0)
                return iRet;
            continue;
        }

        iOffset += iRet;
        iLen -= iRet;
    }

    return iOffset;
}

int CNetClient::Wait(int iEvent, uint32_t dwTimeoutMs)
{
    CCoroutine *pCor = CCoroutine::GetObj();
    CNetTask *pNetTask = (CNetTask *)pCor->GetTaskBase();
    if (!pNetTask)
    {
        m_pFd->SetErr("get net task object context fail");
        return -1;
    }

    int iRet = pNetTask->YieldEventDel(dwTimeoutMs, m_pFd->GetFd(), iEvent, 0);

    if (iRet < 0)
        return -2;

    return 0;
}

int CNetClient::Connect(const char *pszAddr, uint16_t wPort, const char *pszCacert, const char *pszPass, const char *pszCert, const char *pszKey, uint16_t wVer)
{
    m_wProtocol = ITaskBase::PROTOCOL_TCPS;
    m_wVer = wVer;
    CTcpsCli *pCli = new (std::nothrow) CTcpsCli();
    if (!pCli)
    {
        m_pFd->SetErr("new unix object failed");
        return -1;
    }
    m_pFd = pCli;
    CCoroutine *pCor = CCoroutine::GetObj();
    return pCli->Create(pszAddr, wPort, pszCacert, pszPass, pszCert, pszKey, m_dwConnTimeout, pCor->GetTaskBase(), wVer);
}

int CNetClient::TcpsRead(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CTcpsReliableFd *pSock = (CTcpsReliableFd *)m_pFd;
    int iRet = pSock->Read(pszBuf, iLen);
    if (iRet == 0)
        iRet = Wait(ITaskBase::YIELD_ET_IN, dwTimeoutMs);
    return iRet;
}

int CNetClient::TcpsWrite(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CTcpsReliableFd *pSock = (CTcpsReliableFd *)m_pFd;
    int iOffset = 0;
    CPadlock<CCoLock> ol(m_oLock);
    while (iLen > 0)
    {
        int iRet = pSock->Write(pszBuf + iOffset, iLen);
        if (iRet < 0)
            return -1;

        if (iRet == 0)
        {
            if ((iRet = Wait(ITaskBase::YIELD_ET_OUT, dwTimeoutMs)) < 0)
                return iRet;
            continue;
        }

        iOffset += iRet;
        iLen -= iRet;
    }

    return iOffset;
}

void CNetClient::IsOpen()
{
    CSpinLock oLock(m_dwSync);
    if (m_dwCloseRef == 0)
        return;
    ++ m_dwCloseRef;
}

void CNetClient::IsClose()
{
    if (m_dwCloseRef != 0)
    {
        CSpinLock oLock(m_dwSync);
        -- m_dwCloseRef;
        if (m_dwCloseRef != 0)
            return;
    }

    if (m_pFd)
        delete m_pFd;
    m_pFd = 0;
}
