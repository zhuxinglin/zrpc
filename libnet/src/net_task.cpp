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

#include "net_task.h"
#include "socket_fd.h"
#include "task_queue.h"
#include "coroutine.h"
#include "timer_fd.h"
#include "event_fd.h"
#include "schedule.h"
#include "event_epoll.h"
#include "thread.h"
#include "context.h"
#include "libnet.h"

using namespace znet;

extern CContext* g_pContext;

CNetTask::CNetTask() : m_pFd(0), m_wUdpAddLen(0)
{
}

CNetTask::~CNetTask()
{
    if (m_pFd)
    {
        delete m_pFd;
        m_pFd = 0;
    }
}

int CNetTask::Read(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    if (!pszBuf)
        return -1;
    if (m_wProtocol <= ITaskBase::PROTOCOL_UNIX)
        return ReadReliable(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol == ITaskBase::PROTOCOL_TCPS)
        return ReadTcps(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol <= ITaskBase::PROTOCOL_UDPG)
        return ReadUnreliable(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol == ITaskBase::PROTOCOL_TIMER_FD)
        return ReadTimer(pszBuf, iLen);
    else if (m_wProtocol == ITaskBase::PROTOCOL_EVENT_FD)
        return ReadEvent(pszBuf, iLen);
    return -1;
}

int CNetTask::Write(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    if (!pszBuf)
        return -1;
    if (m_wProtocol <= ITaskBase::PROTOCOL_UNIX)
        return WriteReliable(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol == ITaskBase::PROTOCOL_TCPS)
        return WriteTcps(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol <= ITaskBase::PROTOCOL_UDPG)
        return WriteUnreliable(pszBuf, iLen, dwTimeoutMs);
    else if (m_wProtocol == ITaskBase::PROTOCOL_TIMER_FD)
        return WriteTimer(pszBuf, iLen);
    else if (m_wProtocol == ITaskBase::PROTOCOL_EVENT_FD)
        return WriteEvent(pszBuf, iLen);
    return -1;
}

int CNetTask::ReadReliable(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CReliableFd *pSock = (CReliableFd *)m_pFd;
    int iRet;
    while (true)
    {
        iRet = pSock->Read(pszBuf, iLen);
        if (iRet == 0)
        {
            iRet = Yield(dwTimeoutMs);
            if (iRet < 0)
                break;

            continue;
        }
        break;
    }

    return iRet;
}

int CNetTask::ReadUnreliable(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CUnreliableFd *pSock = (CUnreliableFd *)m_pFd;
    m_wUdpAddLen = sizeof(m_szUdpAddr);
    int iRet;
    while (true)
    {
        iRet = pSock->Read(pszBuf, iLen, (struct sockaddr *)m_szUdpAddr, (uint32_t *)&m_wUdpAddLen);
        if (iRet == 0)
        {
            iRet = Yield(dwTimeoutMs);
            if (iRet < 0)
                break;
            continue;
        }
        break;
    }

    return iRet;
}

int CNetTask::WriteReliable(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CReliableFd *pSock = (CReliableFd *)m_pFd;
    int iOffset = 0;
    CPadlock<CCoLock> ol(m_oLock);
    while(iLen > 0)
    {
        int iRet = pSock->Write(pszBuf + iOffset, iLen);
        if (iRet < 0)
            return -1;

        if (iRet == 0)
        {
            iRet = YieldEventRestore(dwTimeoutMs, m_pFd->GetFd(), ITaskBase::YIELD_ET_OUT, ITaskBase::YIELD_ET_IN);
            if (iRet < 0)
                return iRet;
            continue;
        }

        iOffset += iRet;
        iLen -= iRet;
    }

    return iOffset;
}

int CNetTask::WriteUnreliable(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CUnreliableFd *pSock = (CUnreliableFd *)m_pFd;
    int iOffset = 0;
    CPadlock<CCoLock> ol(m_oLock);
    while (iLen > 0)
    {
        int iRet = pSock->Write(pszBuf + iOffset, iLen, (struct sockaddr *)m_szUdpAddr, m_wUdpAddLen);
        if (iRet < 0)
            return -1;

        if (iRet == 0)
        {
            iRet = YieldEventRestore(dwTimeoutMs, m_pFd->GetFd(), ITaskBase::YIELD_ET_OUT, ITaskBase::YIELD_ET_IN);
            if (iRet < 0)
                return iRet;
            continue;
        }

        iOffset += iRet;
        iLen -= iRet;
    }

    return iOffset;
}

int CNetTask::ReadTimer(char *pszBuf, int iLen)
{
    if (iLen < (int)sizeof(uint64_t) || !pszBuf)
        return -1;

    CTimerFd *pTimer = (CTimerFd*)m_pFd;
    int iRet = pTimer->Read((uint64_t *)pszBuf);
    if (iRet < 0)
        return -1;
    return sizeof(uint64_t);
}

int CNetTask::ReadEvent(char *pszBuf, int iLen)
{
    if (iLen < (int)sizeof(uint64_t))
        return -1;
    CEventFd *pEvent = (CEventFd *)m_pFd;
    int iRet = pEvent->Read((uint64_t *)pszBuf);
    if (iRet < 0)
        return -1;
    return sizeof(uint64_t);
}

int CNetTask::WriteTimer(const char *pszBuf, int iLen)
{
    if (iLen < (int)sizeof(uint32_t))
        return -1;

    CTimerFd *pTimer = (CTimerFd *)m_pFd;
    CPadlock<CCoLock> ol(m_oLock);
    int iRet = pTimer->Write(*(int32_t *)pszBuf);
    if (iRet < 0)
        return -1;
    return sizeof(int32_t);
}

int CNetTask::WriteEvent(const char *pszBuf, int iLen)
{
    if (iLen < (int)sizeof(uint64_t))
        return -1;
    CEventFd *pEvent = (CEventFd *)m_pFd;
    CPadlock<CCoLock> ol(m_oLock);
    int iRet = pEvent->Write(*(uint64_t *)pszBuf);
    if (iRet < 0)
        return -1;
    return sizeof(uint64_t);
}

int CNetTask::ReadTcps(char *pszBuf, int iLen, uint32_t dwTimeoutMs)
{
    CTcpsReliableFd *pSock = (CTcpsReliableFd*)m_pFd;
    int iRet;
    while (true)
    {
        iRet = pSock->Read(pszBuf, iLen);
        if (iRet == 0)
        {
            iRet = Yield(dwTimeoutMs);
            if (iRet < 0)
                break;
            continue;
        }
        break;
    }

    return iRet;
}

int CNetTask::WriteTcps(const char *pszBuf, int iLen, uint32_t dwTimeoutMs)
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
            if (YieldEventRestore(dwTimeoutMs, m_pFd->GetFd(), ITaskBase::YIELD_ET_OUT, ITaskBase::YIELD_ET_IN) < 0)
                return -2;
            continue;
        }

        iOffset += iRet;
        iLen -= iRet;
    }

    return iOffset;
}

void CNetTask::Close()
{
    if (m_pFd)
    {
        g_pContext->m_pSchedule->PushMsg(m_pFd->GetFd(), 2, 0, 0);
        m_pFd->Close();
    }

    ITaskBase* pBase = (ITaskBase*)this;
    if (g_pContext->m_pCo->GetTaskBase() != pBase)
        CNet::GetObj()->ExitCo(m_qwCid);
}

void CNetTask::Run()
{
    do
    {
        if (m_wProtocol == ITaskBase::PROTOCOL_TCPS)
        {
            CTcpsReliableFd *pSsl = (CTcpsReliableFd *)m_pFd;
            if (pSsl->Accept(3e3, this) < 0)
                break;
        }
        Go();
    } while (0);
}
