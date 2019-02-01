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

#ifndef __SOCKET_FD__H__
#define __SOCKET_FD__H__

#include "file_fd.h"
#include "task_base.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

namespace znet
{

class CSockFd : public CFileFd
{
public:
    CSockFd();
    ~CSockFd();

public:
    uint32_t GetVer() { return m_dwVer; }
    void SetVer(uint16_t wVer) {m_dwVer = wVer;}
    void SetSync() { m_bAsync = !m_bAsync; }
    int GetFdOpt(int iLevel, int iOptName, void* pOptVal, uint32_t* iOptLen, int iFd = -1);
    int SetFdOpt(int iLevel, int iOptName, void* pOptVal, uint32_t iOptLen, int iFd = -1);

protected:
    int Create(int iAf, int iProtocol);
    int Async(int iFd = -1);
    int GetAddr(const char *pszIpAddr, void **pPtr, int iAf);
    int GetSockAddr(sockaddr_in *addr4, sockaddr_in6 *addr6, sockaddr **pAddr, const char *pszAddr, uint16_t wPort, int* iAddrLen, int iSvc = 0);
    virtual void Close(int iFd = -1);
    int WaitConnect(uint32_t dwTimeout, ITaskBase *pTask, int iEvent = 1);
    int GetConnectErr();
    int Wait(int iEvent, uint32_t dwTimeoutMs, ITaskBase *pTask);
    void SetFdErr(const char* pszDesc, sockaddr_in *addr4, sockaddr_in6 *addr6, const char *pszAddr, uint16_t wPort, uint32_t dwTimeout);

protected:
    uint32_t m_dwVer;
    bool m_bAsync;
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CReliableFd : public CSockFd
{
public:
    CReliableFd();
    ~CReliableFd();

public:
    int Accept(char *pszAddr = 0, int iAddrLen = 0);
    int Read(char *pszBuf, int iBufLen, int iFd = -1);
    int Write(const char *pszBuf, int iBufLen, int iFd = -1);

protected:
    int Create(int dwVer);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CTcpSvc : public CReliableFd
{
public:
    CTcpSvc();
    ~CTcpSvc();

public:
    int Create(const char* pszAddr, uint16_t wPort, uint32_t dwListen, uint32_t dwVer = 4);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CTcpCli : public CReliableFd
{
public:
    CTcpCli();
    ~CTcpCli();

public:
    int Create(const char *pszAddr, uint16_t wPort, uint32_t dwTimeout, ITaskBase *pTask, uint32_t dwVer = 4);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CUnreliableFd : public CSockFd
{
public:
    CUnreliableFd();
    ~CUnreliableFd();

public:
    int Read(char *pszBuf, int iBufLen, sockaddr *pAddr, uint32_t* dwAddLen, int iFd = -1);
    int Write(const char *pszBuf, int iBufLen, sockaddr *pAddr, uint32_t dwAddLen, int iFd = -1);

protected:
    int Create(int dwVer);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CUdpSvc : public CUnreliableFd
{
public:
    CUdpSvc();
    ~CUdpSvc();

public:
    int Create(const char *pszAddr, uint16_t wPort, uint32_t dwGrop = 0, uint32_t dwVer = 4);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CUdpCli : public CUnreliableFd
{
public:
    CUdpCli();
    ~CUdpCli();

public:
    int Create(const char *pszAddr, uint16_t wPort, char* pszAddrInfo, uint32_t *pszAddrLen, uint32_t dwVer = 4);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CUnixSvc : public CReliableFd
{
public:
    CUnixSvc();
    ~CUnixSvc();

public:
    int Create(const char *pszAddr, uint32_t dwListen);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CUnixCli : public CReliableFd
{
public:
    CUnixCli();
    ~CUnixCli();

public:
    int Create(const char *pszAddr, uint32_t dwTimeout, ITaskBase *pTask);
};

//============================================================================
//
//
//
//
//
//
//============================================================================

class CTcpsReliableFd : public CSockFd
{
public:
    CTcpsReliableFd();
    ~CTcpsReliableFd();

public:
    void SetCtx(CTcpsReliableFd *pTcps){m_pCtx = pTcps->m_pCtx;}
    int Read(char *pszBuf, int iBufLen);
    int Write(const char *pszBuf, int iBufLen);
    int Accept(uint32_t dwTimeout, ITaskBase *pTask);

protected:
    virtual void Close(int iFd = -1);
    int X509NameOneline();

private:
    int WaitSslAccept(int iEvent, int iRestoreEvent, uint32_t dwTimeoutMs);

protected:
    SSL_CTX *m_pCtx;
    SSL *m_pSsl;
    ITaskBase *m_pTask;
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CTcpsSvc : public CTcpsReliableFd
{
public:
    CTcpsSvc();
    ~CTcpsSvc();

public:
    int Create(const char* pszAddr, uint16_t wPort, uint32_t dwListen, const char* pszCert, const char* pszKey, uint32_t dwVer = 4);
};

//============================================================================
//
//
//
//
//
//
//============================================================================
class CTcpsCli : public CTcpsReliableFd
{
public:
    CTcpsCli();
    ~CTcpsCli();

public:
    int Create(const char *pszAddr, uint16_t wPort, const char *pszCacert, const char *pszPass,
            const char *pszCert, const char* pszKey, uint32_t dwTimeout, ITaskBase *pTask, uint16_t wVer);
};

}

#endif

