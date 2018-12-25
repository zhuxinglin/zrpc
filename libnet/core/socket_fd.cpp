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

#include "socket_fd.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/time.h>
#include <stddef.h>
#include <netdb.h>
#include <stdio.h>
#include <netinet/tcp.h>

CSockFd::CSockFd() : m_bAsync(true)
{
}

CSockFd::~CSockFd()
{
    Close();
}

void CSockFd::Close(int iFd)
{
    if (iFd == -1)
    {
        iFd = m_iFd;
        m_iFd = -1;
    }

    if (iFd != -1)
    {
        shutdown(iFd, 0);
        close(iFd);
    }
}

int CSockFd::Create(int iAf, int iProtocol)
{
    m_iFd = socket(iAf, iProtocol, 0);
    if (m_iFd < 0)
    {
        SetErr("socket create fail! ", m_iFd);
        return -1;
    }
    return m_iFd;
}

int CSockFd::Async(int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;
    int iFlag = fcntl(iFd, F_GETFL, 0);
    if (iFlag == -1)
    {
        SetErr("get fd mask fail! ", iFd);
        return -1;
    }
    iFlag |= O_NONBLOCK;
    iFlag = fcntl(iFd, F_SETFL, iFlag);
    if (iFlag == -1)
    {
        SetErr("set fd mask sync fail! ", iFd);
        return -1;
    }
    return 0;
}

int CSockFd::GetFdOpt(int iLevel, int iOptName, void* pOptVal, uint32_t *iOptLen, int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;

    int iRet = getsockopt(iFd, iLevel, iOptName, pOptVal, (socklen_t *)iOptLen);
    if (iRet != 0)
    {
        char szBuf[128];
        snprintf(szBuf, sizeof(szBuf), "get socket opt fail!, ret:[%d] level:[%d], optname:[%d], ", iRet, iLevel, iOptName);
        SetErr(szBuf, m_iFd);
    }
    return iRet;
}

int CSockFd::SetFdOpt(int iLevel, int iOptName, void* pOptVal, uint32_t iOptLen, int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;

    int iRet = setsockopt(iFd, iLevel, iOptName, pOptVal, iOptLen);
    if (iRet != 0)
    {
        char szBuf[128];
        snprintf(szBuf, sizeof(szBuf), "set socket opt fail!, ret:[%d] level:[%d], optname:[%d], ", iRet, iLevel, iOptName);
        SetErr(szBuf, m_iFd);
    }
    return iRet;
}

int CSockFd::GetAddr(const char *pszIpAddr, void **pPtr, int iAf)
{
    struct addrinfo hint;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = iAf;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_ADDRCONFIG;

    int iRet = getaddrinfo(pszIpAddr, NULL, &hint, (addrinfo **)pPtr);
    if (0 != iRet || !pPtr)
    {
        SetErr("get addr info fail! ", -1);
        return -1;
    }
    return 0;
}

int CSockFd::GetSockAddr(sockaddr_in *addr4, sockaddr_in6 *addr6, sockaddr **pAddr, const char *pszAddr, uint16_t wPort, int *iAddrLen, int iSvc)
{
    struct addrinfo *pRest = NULL;
    if (4 == m_dwVer)
    {
        addr4->sin_family = AF_INET;
        addr4->sin_port = htons(wPort);
        *iAddrLen = sizeof(*addr4);
        *pAddr = (struct sockaddr *)addr4;
        if (!iSvc)
        {
            if (!pszAddr)
                addr4->sin_addr.s_addr = INADDR_ANY;
            else
                addr4->sin_addr.s_addr = inet_addr(pszAddr);
        }
        else
        {
            if (GetAddr(pszAddr, (void **)&pRest, AF_INET) < 0)
                return -1;
            addr4->sin_addr.s_addr = ((struct sockaddr_in *)(pRest->ai_addr))->sin_addr.s_addr;
        }
    }
    else
    {
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = htons(wPort);
        *iAddrLen = sizeof(*addr6);
        *pAddr = (struct sockaddr *)addr6;
        if (!iSvc)
        {
            if (pszAddr)
                inet_pton(AF_INET6, pszAddr, &addr6->sin6_addr);
            else
                addr6->sin6_addr = in6addr_any;
        }
        else
        {
            if (GetAddr(pszAddr, (void **)&pRest, AF_INET6) < 0)
                return -1;
            memcpy(&addr6->sin6_addr, &((struct sockaddr_in6 *)(pRest->ai_addr))->sin6_addr, sizeof(addr6->sin6_addr));
        }
    }

    if (pRest)
        freeaddrinfo(pRest);
    return 0;
}

int CSockFd::WaitConnect(uint32_t dwTimeout, ITaskBase *pTask, int iEvent)
{
    if (GetConnectErr() == 0)
        return 0;

    if (Wait(iEvent, dwTimeout, pTask) < 0)
        return -1;

    return GetConnectErr();
}

int CSockFd::GetConnectErr()
{
    int iError = 0;
    uint32_t dwLen = sizeof(iError);
    if (GetFdOpt(SOL_SOCKET, SO_ERROR, &iError, &dwLen) < 0)
        iError = -1;

    if (iError == 0)
        return 0;

    return -1;
}

int CSockFd::Wait(int iEvent, uint32_t dwTimeoutMs, ITaskBase *pTask)
{
    if (!pTask)
    {
        usleep(0);
        return 0;
    }

    if (dwTimeoutMs == 0)
        return 0;
    else if (dwTimeoutMs == (uint32_t)-1)
        dwTimeoutMs = 0;

    int iRet = pTask->YieldEventDel(dwTimeoutMs, m_iFd, iEvent, 0);

    if (dwTimeoutMs != 0 && iRet < 0)
        return -1;

    return 0;
}

void CSockFd::SetFdErr(const char *pszDesc, sockaddr_in *addr4, sockaddr_in6 *addr6, const char *pszAddr, uint16_t wPort, uint32_t dwTimeout)
{
    char szBuf[256];
    char szIp[40];
    if (4 == m_dwVer)
        inet_ntop(AF_INET, &addr4->sin_addr, szIp, sizeof(szIp));
    else
        inet_ntop(AF_INET6, &addr6->sin6_addr, szIp, sizeof(szIp));

    snprintf(szBuf, sizeof(szBuf), "%s address:[%s], ip:[%s], prot:[%d], ver:[%u], timeout:[%u]", pszDesc, pszAddr, szIp, wPort, m_dwVer, dwTimeout);
    SetErr(szBuf, m_iFd);
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CReliableFd::CReliableFd()
{
}

CReliableFd::~CReliableFd()
{
}

int CReliableFd::Accept(char *pszAddr, int iAddrLen)
{
    socklen_t dwLen = 0;
    int iFd;
    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    struct sockaddr *pAddr = NULL;
    if (pszAddr)
    {
        if (4 == m_dwVer)
        {
            dwLen = sizeof(struct sockaddr_in);
            pAddr = (sockaddr *)&addr4;
        }
        else
        {
            dwLen = sizeof(struct sockaddr_in6);
            pAddr = (sockaddr *)&addr6;
        }
    }
    iFd = ::accept(m_iFd, pAddr, &dwLen);

    if (iFd == -1)
    {
        SetErr("accept new connect fail! ", m_iFd);
        return -1;
    }

    if (Async() < 0)
    {
        ::close(iFd);
        return -1;
    }

    if (pszAddr)
    {
        if (4 == m_dwVer)
            inet_ntop(AF_INET, &addr4.sin_addr, pszAddr, iAddrLen);
        else
            inet_ntop(AF_INET6, &addr6.sin6_addr, pszAddr, iAddrLen);
    }

    return iFd;
}

int CReliableFd::Read(char *pszBuf, int iBufLen, int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;

    while (!m_bAsync)
    {
        errno = 0;
        int iRet = recv(iFd, pszBuf, iBufLen, 0);
        if (iRet < 0 && (errno == EINTR || errno == EAGAIN))
            continue;

        if (iRet == 0)
            iRet = -1;

        return iRet;
    }

    errno = 0;
    int iRet = recv(iFd, pszBuf, iBufLen, MSG_DONTWAIT);
    if (iRet == -1 && (errno == EINTR || errno == EAGAIN))
        return 0;

    if (0 == iRet)
        iRet = -1;

    return iRet;
}

int CReliableFd::Write(const char *pszBuf, int iBufLen, int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;

    while (!m_bAsync)
    {
        errno = 0;
        int iRet = send(iFd, pszBuf, iBufLen, 0);
        if (-1 == iRet && (errno == EAGAIN || errno == EINTR))
            continue;

        return iRet;
    }

    errno = 0;
    int iRet = send(iFd, pszBuf, iBufLen, MSG_NOSIGNAL | MSG_DONTWAIT);
    if (-1 == iRet && (errno == EAGAIN || errno == EINTR))
    {
        // Buffer overflow
        iRet = 0;
    }
    return iRet;
}

int CReliableFd::Create(int dwVer)
{
    m_dwVer = dwVer;
    int iAf = AF_INET;
    if (dwVer != 4)
        iAf = AF_INET6;

    if (CSockFd::Create(iAf, SOCK_STREAM) < 0)
    {
        return -1;
    }
    return 0;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CTcpSvc::CTcpSvc()
{
}

CTcpSvc::~CTcpSvc()
{
}

int CTcpSvc::Create(const char *pszAddr, uint16_t wPort, uint32_t dwListen, uint32_t dwVer)
{
    if (CReliableFd::Create(dwVer) < 0)
    {
        return -1;
    }

    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    struct sockaddr *pAddr;
    int iAddrLen;
    int iReuse = 1;

    do
    {
        if (Async() < 0)
            break;

        GetSockAddr(&addr4, &addr6, &pAddr, pszAddr, wPort, &iAddrLen);
        if (SetFdOpt(SOL_SOCKET, SO_REUSEADDR, &iReuse, sizeof(iReuse)) < 0)
        {
            break;
        }

        if (::bind(m_iFd, pAddr, iAddrLen) < 0)
        {
            SetFdErr("bind socket fail!", &addr4, &addr6, pszAddr, wPort, 0);
            break;
        }

        if (listen(m_iFd, dwListen) < 0)
        {
            SetFdErr("listen socket fail!", &addr4, &addr6, pszAddr, wPort, 0);
            break;
        }

        return m_iFd;
    } while (0);
    ::close(m_iFd);
    m_iFd = -1;
    return -1;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CTcpCli::CTcpCli()
{
}

CTcpCli::~CTcpCli()
{
}

int CTcpCli::Create(const char *pszAddr, uint16_t wPort, uint32_t dwTimeout, ITaskBase *pTask, uint32_t dwVer)
{
    if (CReliableFd::Create(dwVer) < 0)
    {
        return -1;
    }

    struct sockaddr_in addr4;
    struct sockaddr_in6 addr6;
    struct sockaddr *pAddr;
    int iAddrLen;
    do
    {
        GetSockAddr(&addr4, &addr6, &pAddr, pszAddr, wPort, &iAddrLen, 1);
        struct timeval tv;
        tv.tv_sec = 3;
        tv.tv_usec = 0;

        if (SetFdOpt(SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        {
            break;
        }

        tv.tv_sec = 3;
        if (SetFdOpt(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            break;
        }

        if (m_bAsync && Async() < 0)
            break;

        errno = 0;
        if (::connect(m_iFd, pAddr, iAddrLen) < 0)
        {
            if (EINPROGRESS != errno && EWOULDBLOCK != errno)
            {
                SetFdErr("connect server fail!", &addr4, &addr6, pszAddr, wPort, dwTimeout);
                break;
            }
        }
/*
        int iKeepAlive = 1;
        // 开启keepalive
        if (SetFdOpt(SOL_SOCKET, SO_KEEPALIVE, &iKeepAlive, sizeof(iKeepAlive)) < 0)
        {
            break;
        }

        int iKeepIdle = 30; //默认如该连接在5秒内没有任何数据往来，则进行探测
        if (SetFdOpt(SOL_TCP, TCP_KEEPIDLE, &iKeepIdle, sizeof(iKeepIdle)) < 0)
        {
            break;
        }

        int iKeepInterval = 2; //默认探测时发包的时间间隔为2秒
        if (SetFdOpt(SOL_TCP, TCP_KEEPINTVL, &iKeepInterval, sizeof(iKeepInterval)) < 0)
        {
            break;
        }

        int iKeepCount = 3; //默认探测尝试的次数。如果第1次探测包就收到响应了，则后2次的不再发送
        if (SetFdOpt(SOL_TCP, TCP_KEEPCNT, &iKeepCount, sizeof(iKeepCount)) < 0)
        {
            break;
        }
*/

        if (!m_bAsync && WaitConnect(dwTimeout, pTask) < 0)
        {
            SetFdErr("connect server timeout!", &addr4, &addr6, pszAddr, wPort, dwTimeout);
            break;
        }

        return m_iFd;
    } while (0);
    ::close(m_iFd);
    m_iFd = -1;
    return -1;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CUnreliableFd::CUnreliableFd()
{
}

CUnreliableFd::~CUnreliableFd()
{
}

int CUnreliableFd::Read(char *pszBuf, int iBufLen, sockaddr *pAddr, uint32_t *dwAddLen, int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;
    errno = 0;
    // 接收网络数据包
    int iRecvLen = recvfrom(iFd, pszBuf, iBufLen, MSG_DONTWAIT, pAddr, (socklen_t *)dwAddLen);
    if (-1 == iRecvLen && (errno == EAGAIN || errno == EINTR))
    {
        return 0;
    }
    if (0 == iRecvLen)
        return -1;

    return iRecvLen;
}

int CUnreliableFd::Write(const char *pszBuf, int iBufLen, sockaddr *pAddr, uint32_t dwAddLen, int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;
    errno = 0;
    int iSsendLen = sendto(iFd, pszBuf, iBufLen, MSG_NOSIGNAL | MSG_DONTWAIT, pAddr, dwAddLen);
    if (iSsendLen < 0 && (errno == EAGAIN || EINTR == errno))
    {
        iSsendLen = 0;
    }
    return iSsendLen;
}

int CUnreliableFd::Create(int dwVer)
{
    m_dwVer = dwVer;
    int iAf = AF_INET;
    if (dwVer != 4)
        iAf = AF_INET6;

    if (CSockFd::Create(iAf, SOCK_DGRAM) < 0)
    {
        return -1;
    }
    return 0;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CUdpSvc::CUdpSvc()
{
}

CUdpSvc::~CUdpSvc()
{
}

int CUdpSvc::Create(const char *pszAddr, uint16_t wPort, uint32_t dwGrop, uint32_t dwVer)
{
    if (CUnreliableFd::Create(dwVer) < 0)
    {
        return -1;
    }

    do
    {
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        struct sockaddr *pAddr;
        int iAddrLen;

        GetSockAddr(&addr4, &addr6, &pAddr, pszAddr, wPort, &iAddrLen);

        int iReuse = 1;
        if (SetFdOpt(SOL_SOCKET, SO_REUSEADDR, &iReuse, sizeof(iReuse)) < 0)
            break;

        if (::bind(m_iFd, pAddr, iAddrLen) < 0)
        {
            SetFdErr("bind socket fail!", &addr4, &addr6, pszAddr, wPort, 0);
            break;
        }

        if (Async() < 0)
            break;

        /*
        if (dwGrop == 1)
        {
            int iProco;
            int iOptAdd;
            int iOptLoop;

            void* pReq;
            int iReqLen;
            if (dwVer == 4)
            {
                struct ip_mreq mreq;
                if (!ip_addr)
                    mreq.imr_interface.s_addr = INADDR_ANY;
                else
                    mreq.imr_interface.s_addr = inet_addr(ip_addr);
                mreq.imr_multiaddr.s_addr = inet_addr(gaddr);

                iProco = IPPROTO_IP;
                iOptAdd = IP_ADD_MEMBERSHIP;
                iOptLoop = IP_MULTICAST_LOOP;
                pReq = &mreq;
                iReqLen = sizeof(mreq);
            }
            else
            {
                struct ipv6_mreq mreq;

                iProco = IPPROTO_IPV6;
                iOptAdd = IPV6_ADD_MEMBERSHIP;
                iOptLoop = IPV6_MULTICAST_LOOP;
                pReq = &mreq;
                iReqLen = sizeof(mreq);
            }

            if (setsockopt(m_iFd, iProco, iOptAdd, pReq, iReqLen) < 0)
            {
                break;
            }

            int iLoop = 0;
            if (setsockopt(m_iFd, iProco, iOptLoop, &iLoop, sizeof(iLoop)) < 0)
            {
                break;
            }

            int iTtl = 1;
            if (setsockopt(m_iFd, iProco, IP_MULTICAST_TTL, &iTtl, sizeof(iTtl)) < 0)
            {
                break;
            }

            if (::connect(m_iFd, pAddr, iAddrLen) < 0)
            {
                SetFdErr("connect bind address faild!", &addr4, &addr6, pszAddr, wPort, 0);
                break;
            }
        }
*/

        return m_iFd;
    } while (0);
    close(m_iFd);
    m_iFd = -1;
    return -1;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CUdpCli::CUdpCli()
{
}

CUdpCli::~CUdpCli()
{
}

int CUdpCli::Create(const char *pszAddr, uint16_t wPort, char *pszAddrInfo, uint32_t *pszAddrLen, uint32_t dwVer)
{
    if (CUnreliableFd::Create(dwVer) < 0)
    {
        return -1;
    }

    do
    {
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
        struct sockaddr *pAddr;
        int iAddrLen;

        if (GetSockAddr(&addr4, &addr6, &pAddr, pszAddr, wPort, &iAddrLen, 1) < 0)
        {
            break;
        }

        if (dwVer == 4)
        {
            *pszAddrLen = sizeof(addr4);
            memcpy(pszAddrInfo, &addr4, sizeof(addr4));
        }
        else
        {
            *pszAddrLen = sizeof(addr6);
            memcpy(pszAddrInfo, &addr6, sizeof(addr6));
        }
        

        if (::connect(m_iFd, pAddr, iAddrLen) < 0)
        {
            SetFdErr("connect bind address faild!", &addr4, &addr6, pszAddr, wPort, 0);
            break;
        }

        if (Async() < 0)
            break;

        return m_iFd;
    } while (0);
    close(m_iFd);
    m_iFd = -1;
    return -1;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CUnixSvc::CUnixSvc()
{
}

CUnixSvc::~CUnixSvc()
{
}

int CUnixSvc::Create(const char *pszAddr, uint32_t dwListen)
{
    if (CSockFd::Create(AF_UNIX, SOCK_STREAM) < 0)
    {
        return -1;
    }

    int iLen;
    struct sockaddr_un oUnAddr;
    oUnAddr.sun_family = AF_UNIX;
    strcpy(oUnAddr.sun_path, pszAddr);
    iLen = offsetof(sockaddr_un, sun_path) + strlen(pszAddr);
    do
    {
        int iReuse = 1;
        if (SetFdOpt(SOL_SOCKET, SO_REUSEADDR, &iReuse, sizeof(iReuse)) < 0)
        {
            break;
        }

        if (::bind(m_iFd, (sockaddr *)&oUnAddr, iLen) < 0)
        {
            char szBuf[128];
            snprintf(szBuf, sizeof(szBuf), "bind unix fail! addr:[%s], ", pszAddr);
            SetErr(szBuf, m_iFd);
            break;
        }

        if (listen(m_iFd, dwListen) < 0)
        {
            char szBuf[128];
            snprintf(szBuf, sizeof(szBuf), "bind unix fail! addr:[%s], listen:[%u], ", pszAddr, dwListen);
            SetErr(szBuf, m_iFd);
            break;
        }

        if (Async() < 0)
            break;

        return m_iFd;
    } while (0);
    ::close(m_iFd);
    m_iFd = -1;

    return -1;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CUnixCli::CUnixCli()
{
}

CUnixCli::~CUnixCli()
{
}

int CUnixCli::Create(const char *pszAddr, uint32_t dwTimeout, ITaskBase *pTask)
{
    if (CSockFd::Create(AF_UNIX, SOCK_STREAM) < 0)
    {
        return -1;
    }

    int iLen;
    struct sockaddr_un oUnaddr;
    oUnaddr.sun_family = AF_UNIX;
    strcpy(oUnaddr.sun_path, pszAddr);
    iLen = offsetof(struct sockaddr_un, sun_path) + strlen(pszAddr);
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    do
    {
        if (SetFdOpt(SOL_SOCKET, SO_SNDTIMEO, &tv, (socklen_t)sizeof(tv)) < 0)
            break;

        tv.tv_sec = 2;
        tv.tv_usec = 0;
        if (SetFdOpt(SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
            break;

        if (m_bAsync && Async() < 0)
            break;

        if (::connect(m_iFd, (sockaddr *)&oUnaddr, iLen) < 0)
        {
            char szBuf[128];
            snprintf(szBuf, sizeof(szBuf), "connect unix fail! addr:[%s], ", pszAddr);
            SetErr(szBuf, m_iFd);
            break;
        }

        if (m_bAsync && WaitConnect(dwTimeout, pTask) < 0)
        {
            char szBuf[128];
            snprintf(szBuf, sizeof(szBuf), "connect unix timeout! addr:[%s], timeout:[%u], ", pszAddr, dwTimeout);
            SetErr(szBuf, m_iFd);
            break;
        }

        return m_iFd;
    } while (0);
    ::close(m_iFd);
    m_iFd = -1;
    return -1;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CTcpsReliableFd::CTcpsReliableFd() : m_pCtx(0),
                                     m_pSsl(0)
{
}

CTcpsReliableFd::~CTcpsReliableFd()
{
    Close();
}

void CTcpsReliableFd::Close(int iFd)
{
    if (iFd == -1)
        iFd = m_iFd;

    // 关闭连接
    if (m_pSsl)
    {
        int iMode = SSL_RECEIVED_SHUTDOWN | SSL_SENT_SHUTDOWN;
        SSL_set_shutdown(m_pSsl, iMode);
/*
        SSL_set_quiet_shutdown(m_pSsl, 1);
        int n, iSslErr;
        while ((n = SSL_shutdown(m_pSsl)) != 1)
        {
            iSslErr = SSL_get_error(m_pSsl, n);
            //printf("++++++++++++++++++++++++++++++++++ %d  %d  %d  %d\n", n, iSslErr, SSL_ERROR_WANT_WRITE, SSL_ERROR_WANT_READ);
            if (!m_bAsync)
            {
                if (iSslErr == 0 || iSslErr == SSL_ERROR_ZERO_RETURN)
                    break;
                continue;
            }

            if (iSslErr == SSL_ERROR_WANT_WRITE)
            {
                if (Wait(ITaskBase::YIELD_ET_OUT, 3e3, m_pTask) < 0)
                    break;
            }
            else if (iSslErr == SSL_ERROR_WANT_READ)
            {
                if (Wait(ITaskBase::YIELD_ET_IN, 3e3, m_pTask) < 0)
                    break;
            }
            else
                break;
        }
*/
        SSL_free(m_pSsl);
    }
    m_pSsl = NULL;

    if (iFd != -1)
    {
        shutdown(iFd, 0);
        close(iFd);
    }
    m_iFd = -1;
}

int CTcpsReliableFd::Read(char *pszBuf, int iBufLen)
{
    int iRes = SSL_read(m_pSsl, pszBuf, iBufLen);
    if (iRes <= 0)
    {
        if (SSL_ERROR_WANT_READ == SSL_get_error(m_pSsl, iRes))
            return 0;
        return -1;
    }
    
    return iRes;
}

int CTcpsReliableFd::Write(const char *pszBuf, int iBufLen)
{
    int iRes = SSL_write(m_pSsl, pszBuf, iBufLen);
    if (iRes <= 0)
    {
        if (SSL_ERROR_WANT_WRITE == SSL_get_error(m_pSsl, iRes))
            return 0;
        return -1;
    }
    return iRes;
}

int CTcpsReliableFd::Accept(uint32_t dwTimeout, ITaskBase *pTask)
{
    m_pSsl = SSL_new(m_pCtx);
    if (!m_pSsl)
    {
        SetErr("SSL_new failed");
        return -1;
    }

    int iRes = SSL_set_fd(m_pSsl, m_iFd);
    if (iRes != 1)
    {
        SetErr("SSL_set_fd failed");
        return -1;
    }
    m_pTask = pTask;
    SSL_set_accept_state(m_pSsl);
    while ((iRes = SSL_do_handshake(m_pSsl)) != 1)
    {
        int iErr = SSL_get_error(m_pSsl, iRes);
        if (iErr == SSL_ERROR_WANT_WRITE)
        {
            if (WaitSslAccept(ITaskBase::YIELD_ET_OUT, ITaskBase::YIELD_ET_IN, dwTimeout) < 0)
            {
                SetErr("SSL_accept conttect timeout");
                return -1;
            }
        }
        else if (iErr == SSL_ERROR_WANT_READ)
        {
            if (WaitSslAccept(-1, -1, dwTimeout) < 0)
            {
                SetErr("SSL_accept conttect timeout");
                return -1;
            }
        }
        else
        {
            SetErr("SSL_accept failed");
            return -1;
        }
    }
    return X509NameOneline() < 0 ? -1 : m_iFd;
}

int CTcpsReliableFd::WaitSslAccept(int iSetEvent, int iRestoreEvent, uint32_t dwTimeoutMs)
{
    if (dwTimeoutMs == 0)
        return 0;
    else if (dwTimeoutMs == (uint32_t)-1)
        dwTimeoutMs = 0;

    int iRet = m_pTask->YieldEventRestore(dwTimeoutMs, m_iFd, iSetEvent, iRestoreEvent);

    if (dwTimeoutMs != 0 && iRet < 0)
        return -1;

    return 0;
}

int CTcpsReliableFd::X509NameOneline()
{
    X509* pCert;
    pCert = SSL_get_peer_certificate(m_pSsl);
    if (!pCert)
        return 0;

    int iRet = -1;
    do
    {
        char *pStr = X509_NAME_oneline(X509_get_subject_name(pCert), 0, 0);
        if (!pStr)
        {
            SetErr("X509_NAME_oneline X509_get_subject_name failed");
            break;
        }
        OPENSSL_free(pStr);

        pStr = X509_NAME_oneline(X509_get_issuer_name(pCert), 0, 0);
        if (!pStr)
        {
            SetErr("X509_NAME_oneline X509_get_issuer_name failed");
            break;
        }
        iRet = 0;
    }while (0);
    X509_free(pCert);
    return iRet;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CTcpsSvc::CTcpsSvc()
{
}

CTcpsSvc::~CTcpsSvc()
{
    // 释放ssl资源
    if (m_pCtx)
        SSL_CTX_free(m_pCtx);
    m_pCtx = NULL;
}

int CTcpsSvc::Create(const char *pszAddr, uint16_t wPort, uint32_t dwListen, const char *pszCert, const char *pszKey, uint32_t dwVer)
{
    if (!pszCert || !pszKey)
    {
        SetErr("ssl cert or key empty check fail");
        return -1;
    }
    CTcpSvc oSvc;
    if (oSvc.Create(pszAddr, wPort, dwListen, dwVer) < 0)
        return -1;

    m_iFd = oSvc.GetFd();
    oSvc.SetFd(-1);

    m_pCtx = SSL_CTX_new(SSLv23_server_method());
    if (!m_pCtx)
    {
        SetErr("function SSL_CTX_new fail");
        return -1;
    }
    SSL_CTX_set_options(m_pCtx, SSL_OP_ALL);
    SSL_CTX_set_quiet_shutdown(m_pCtx, 1);

    int iRet = SSL_CTX_use_certificate_file(m_pCtx, pszCert, SSL_FILETYPE_PEM);
    if (iRet != 1)
    {
        char szBuf[128];
        snprintf(szBuf, sizeof(szBuf), "SSL_CTX_use_certificate_file '%s' failed", pszCert);
        SetErr(szBuf);
        return -1;
    }
    iRet = SSL_CTX_use_PrivateKey_file(m_pCtx, pszKey, SSL_FILETYPE_PEM);
    if (iRet != 1)
    {
        char szBuf[128];
        snprintf(szBuf, sizeof(szBuf), "SSL_CTX_use_PrivateKey_file '%s' failed", pszKey);
        SetErr(szBuf);
        return -1;
    }

    iRet = SSL_CTX_check_private_key(m_pCtx);
    if (iRet != 1)
    {
        SetErr("SSL_CTX_check_private_key failed");
        return -1;
    }
    return m_iFd;
}

//============================================================================
//
//
//
//
//
//
//============================================================================
CTcpsCli::CTcpsCli()
{

}

CTcpsCli::~CTcpsCli()
{
    // 释放ssl资源
    if (m_pCtx)
        SSL_CTX_free(m_pCtx);
    m_pCtx = NULL;
}

int CTcpsCli::Create(const char *pszAddr, uint16_t wPort, const char *pszCacert, const char *pszPass,
                     const char *pszCert, const char *pszKey, uint32_t dwTimeout, ITaskBase *pTask, uint16_t wVer)
{
    m_dwVer = wVer;
    CTcpCli oCli;
    oCli.SetSync();
    if (oCli.Create(pszAddr, wPort, dwTimeout, pTask, wVer) < 0)
    {
        return -1;
    }

    m_iFd = oCli.GetFd();
    oCli.SetFd(-1);

    m_pCtx = SSL_CTX_new(SSLv23_client_method());
    if (!m_pCtx)
    {
        SetErr("new ssl CTX fail");
        return -1;
    }
    SSL_CTX_set_options(m_pCtx, SSL_OP_ALL);
    SSL_CTX_set_quiet_shutdown(m_pCtx, 1);

    m_pSsl = SSL_new(m_pCtx);
    if (!m_pSsl)
    {
        SetErr("new ssl object fail");
        return -1;
    }

    // 加载本地证书
    if (pszCacert)
    {
        if (SSL_CTX_load_verify_locations(m_pCtx, pszCacert, NULL) != 1)
        {
            SetErr("SSL_CTX_load_verify_locations failed!");
            return -1;
        }

        // 设置为验证对方
        SSL_CTX_set_verify(m_pCtx, SSL_VERIFY_PEER, NULL);
        SSL_CTX_set_verify_depth(m_pCtx, 1);
    }

    if (pszPass)
        SSL_CTX_set_default_passwd_cb_userdata(m_pCtx, (void *)pszPass);

    if (pszCert && pszKey)
    {
        //加载本地证书文件
        if (1 != SSL_CTX_use_certificate_file(m_pCtx, pszCert, SSL_FILETYPE_PEM))
        {
            SetErr("SSL_CTX_use_certificate_file failed!");
            return -1;
        }

        // 加载私钥文件
        if (0 != SSL_CTX_use_PrivateKey_file(m_pCtx, pszKey, SSL_FILETYPE_PEM))
        {
            SetErr("SSL_CTX_use_PrivateKey_file failed!");
            return -1;
        }

        // 检查证书和私钥是否匹配
        if (SSL_CTX_check_private_key(m_pCtx) != 1)
        {
            SetErr("Private key does not match the certificate public key");
            return -1;
        }
    }

    // 设置fd
    SSL_set_fd(m_pSsl, m_iFd);

    // 设置为自动重新连接服务模式
    SSL_set_mode(m_pSsl, SSL_MODE_AUTO_RETRY);

    // 连接验证服务器
    int iRes;

    if (!m_bAsync)
    {
        iRes = SSL_connect(m_pSsl);
        if (SSL_ERROR_NONE != SSL_get_error(m_pSsl, iRes))
        {
            SetErr("SSL_connect failed");
            return -1;
        }
        return 0;
    }

    m_pTask = pTask;
    SSL_set_connect_state(m_pSsl);

    while ((iRes = SSL_do_handshake(m_pSsl)) != 1)
    {
        int iErr = SSL_get_error(m_pSsl, iRes);
        if (iErr == SSL_ERROR_WANT_WRITE)
        {
            if (Wait(ITaskBase::YIELD_ET_OUT, dwTimeout, pTask) < 0)
            {
                SetErr("SSL_connect conttect timeout");
                return -1;
            }
        }
        else if (iErr == SSL_ERROR_WANT_READ)
        {
            if (Wait(ITaskBase::YIELD_ET_IN, dwTimeout, pTask) < 0)
            {
                SetErr("SSL_connect conttect timeout");
                return -1;
            }
        }
        else
        {
            SetErr("SSL_connect failed");
            return -1;
        }
    }
    return X509NameOneline() < 0 ? -1 : 0;
}