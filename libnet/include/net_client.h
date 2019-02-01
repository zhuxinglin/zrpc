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
    int Read(char* pszBuf, int ilen);
    int Write(const char* pszBuf, int ilen);
    void SetConnTimeoutMs(uint32_t dwTimeoutMs){m_dwConnTimeout = dwTimeoutMs;}
    void SetReadTimeoutMs(uint32_t dwTimeoutMs){m_dwReadTimeout = dwTimeoutMs;}
    std::string GetErr(){return m_pFd->GetErr();};

private:
    int TcpConnect(const char* pszAddr, uint16_t wPort);
    int UdpConnect(const char *pszAddr, uint16_t wPort);
    int UnixConnect(const char *pszAddr);
    int ReadReliable(char* pszBuf, int iLen);
    int ReadUnreliable(char* pszBuf, int iLen);
    int WriteReliable(const char* pszBuf, int iLen);
    int WriteUnreliable(const char* pszBuf, int iLen);
    int Wait(int iEvent, uint32_t dwTimeoutMs = 0);
    int TcpsRead(char *pszBuf, int iLen);
    int TcpsWrite(const char* pszBuf, int iLen);

private:
    uint16_t m_wProtocol;
    uint16_t m_wVer;
    uint32_t m_dwConnTimeout;
    uint32_t m_dwReadTimeout;
    char m_szUdpAddr[32];
    uint16_t m_wAddrLen;
    CFileFd *m_pFd;
};

}

#endif
