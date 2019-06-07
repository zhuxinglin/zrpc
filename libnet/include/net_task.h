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

#ifndef __NET_TASK_H__
#define __NET_TASK_H__

#include "task_base.h"
#include "file_fd.h"
#include "co_lock.h"

namespace znet
{

class CNetTask : public ITaskBase
{
public:
    CNetTask();
    ~CNetTask();

public:
    int Read(char* pszBuf, int iLen);
    int Write(const char* pszBuf, int iLen);
    void Sleep(uint32_t dwTimeoutMs = 0);
#define yield()       Sleep()
    void Close();

protected:
    virtual void Go() = 0;

private:
    virtual void Run() FINAL;
    int ReadReliable(char* pszBuf, int iLen);
    int ReadUnreliable(char* pszBuf, int iLen);
    int WriteReliable(const char* pszBuf, int iLen);
    int WriteUnreliable(const char* pszBuf, int iLen);
    int ReadTimer(char* pszBuf, int iLen);
    int ReadEvent(char* pszBuf, int iLen);
    int WriteTimer(const char* pszBuf, int iLen);
    int WriteEvent(const char* pszBuf, int iLen);
    int ReadTcps(char *pszBuf, int iLen);
    int WriteTcps(const char* pszBuf, int iLen);

public:
    CFileFd *m_pFd;
    char m_szAddr[40];
    char m_szUdpAddr[32];
    uint8_t m_wUdpAddLen;
    CCoLock m_oLock;
};

}

#endif
