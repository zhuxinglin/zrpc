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

#ifndef __LIB_NET_H__
#define __LIB_NET_H__

#include "task_base.h"
#include "event_epoll.h"
#include <string>
#include "memory_pool.h"
#include "net_task.h"
#include "file_fd.h"

class CNet
{
public:
    CNet();
    ~CNet();

public:
    int Init(uint32_t dwWorkThread = 0, uint32_t dwSp = 0);
    int Register(NEWOBJ(ITaskBase, pNewObj), void *pData, uint16_t wProtocol, uint16_t wPort, const char *pszIP,
                 uint16_t wVer, uint32_t dwTimeoutMs, const char* pszServerName, const char *pszSslCert = 0, const char *pszSslKey = 0);
    int Register(NEWOBJ(ITaskBase, pNewObj), void* pData, uint16_t wProtocol, uint32_t dwTimeoutUs);
    int Register(ITaskBase *pBase, void* pData, uint16_t wProtocol, int iFd, uint32_t dwTimeoutMs);
    int Start();
    int Unregister(const char *pszServerName);
    const char* GetErr(){return m_sErr.c_str();}

private:
    CFileFd* GetFd(uint16_t wProtocol, uint16_t wPort, const char *pszIP, uint16_t wVer, const char *pszSslCert, const char *pszSslKey);
    int SetUdpTask(NEWOBJ(ITaskBase, pNewObj), CFileFd* pFd, void *pData, uint16_t wProtocol, const char *pszIP);
    int Go();
    ITaskBase *NewTask(NEWOBJ(ITaskBase, pNewObj), void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs = -1);
    ITaskBase *NewTask(ITaskBase *pBase, void *pData, uint16_t wProtocol, uint32_t dwTimeoutUs);
    int AddTimerTask(ITaskBase* pTask, uint32_t dwTimeout);
    int AddFdTask(ITaskBase* pTask, int iFd);
    CFileFd* GetFileFd(uint16_t wProtocol, int iFd);
    int FreeListenFd(void* pEvent, void* pData);
    int RemoveServer(void* pEvent, void* pData);

private:
    CEventEpoll m_oEvent;
    CMemoryPool m_oNetPool;
    std::string m_sErr;
};


#endif