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

#ifndef __NET_CLIENT_H__
#define __NET_CLIENT_H__

#include "socket_fd.h"
#include "task_base.h"
#include "co_lock.h"

namespace znet
{

class CNetClient
{
public:
    CNetClient();
    ~CNetClient();

public:
    int Connect(const char* pszAddr, uint16_t wPort, uint16_t wProtocol = ITaskBase::PROTOCOL_TCP, uint16_t wVer = 4);
    int Connect(const char *pszAddr, uint16_t wPort, const char *pszCacert, const char *pszPass, const char *pszCert, const char* pszKey, uint16_t wVer = 4);
    int Read(char *pszBuf, int ilen, uint32_t dwTimeoutMs = 0xFFFFFFFF);
    int Write(const char *pszBuf, int ilen, uint32_t dwTimeoutMs = 0xFFFFFFFF);
    void SetConnTimeoutMs(uint32_t dwTimeoutMs){m_dwConnTimeout = dwTimeoutMs;}
    std::string GetErr(){return m_pFd->GetErr();};

private:
    int TcpConnect(const char* pszAddr, uint16_t wPort);
    int UdpConnect(const char *pszAddr, uint16_t wPort);
    int UnixConnect(const char *pszAddr);
    int ReadReliable(char *pszBuf, int iLen, uint32_t dwTimeoutMs);
    int ReadUnreliable(char *pszBuf, int iLen, uint32_t dwTimeoutMs);
    int WriteReliable(const char *pszBuf, int iLen, uint32_t dwTimeoutMs);
    int WriteUnreliable(const char *pszBuf, int iLen, uint32_t dwTimeoutMs);
    int Wait(int iEvent, uint32_t dwTimeoutMs = 0);
    int TcpsRead(char *pszBuf, int iLen, uint32_t dwTimeoutMs);
    int TcpsWrite(const char *pszBuf, int iLen, uint32_t dwTimeoutMs);

private:
    uint16_t m_wProtocol;
    uint16_t m_wVer;
    uint32_t m_dwConnTimeout;
    char m_szUdpAddr[32];
    uint16_t m_wAddrLen;
    CFileFd *m_pFd;
    CCoLock m_oLock;
};

}

#endif
